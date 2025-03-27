/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 *
 * Gameplay implementation
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "SMSlib.h"

#define TARGET_SMS
#include "bank_2.h"
#include "bank_3.h"
#include "../game_tile_data/palette.h"

#include "vram.h"
#define INCLUDE_CARD_DATA
#include "cards.h"
#include "castle.h"
#include "panel.h"
#include "game.h"
#include "sound.h"

#define MIN(X,Y) (((X) < (Y)) ? X : Y)

/* Pattern and index data */
extern const uint16_t background_indices [];
uint16_t vdp_background_indices [432]; /* 192 × 144 area */

/* External functions */
extern void card_buffer_prepare (void);
extern void card_slide_from (uint16_t start_x, uint16_t start_y, card_t card, uint8_t slot);
extern void card_slide_to (uint16_t end_x, uint16_t end_y);
extern void card_slide_to_fast (uint16_t end_x, uint16_t end_y);
extern void card_slide_done (void);
extern void render_card_as_background (uint8_t x, uint8_t y, card_t card, uint8_t slot);
extern void delay_frames (uint8_t frames);

/* Shared data */
uint16_t player_patterns_start = 0;
uint16_t panel_patterns_start = 0;
extern bool reset;

/* Game Settings */
bool infinite_game = false;
bool player_visible [2] = { true, false };
bool player_human [2] = { true, false };

/* Game State */
uint16_t wins [2] = { 0, 0 };
uint8_t player = 0;
card_t hands [2] [8];
uint16_t resources [2] [FIELD_MAX];
uint8_t empty_slot = 0;
bool castle_damaged = false;

/* A blank pattern */
static const uint32_t blank_pattern [8] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

/* Starting resources */
const uint16_t starting_resources [8] = {
    [BUILDERS] = 2,
    [BRICKS]   = 5,
    [SOLDIERS] = 2,
    [WEAPONS]  = 5,
    [MAGI]     = 2,
    [CRYSTALS] = 5,
    [CASTLE]   = 30,
    [FENCE]    = 10
};


/*
 * Draw a card and place it into the active player's hand.
 */
static void draw_card (uint8_t slot, bool fast)
{
    uint16_t r = rand ();
    card_t card = 0;

    /* Probability ranges derived from the original game, using:
     *
     *     for (uint32_t i = 0x0000; i <= 32767; i++)
     *     {
     *         int card = 30 * pow ((double) i / 0x10000), 1.6);
     *         card = card / 3 + (card % 3) * 10;
     *         count [card] += 1;
     *     }
     *
     * Note that 32767 is RAND_MAX for SDCC.
     */
    const uint16_t ranges [30] = {
         3911 , 5442 , 6654,  7705 , 8653, 9527,  10346, 11119, 11855, 12561,
        14682, 16074, 17223, 18235, 19156, 20011, 20813, 21574, 22300, 22996,
        24735, 26025, 27121, 28099, 28996, 29831, 30619, 31367, 32082, 0xffff
    };

    for (card = 0; card < 30; card++)
    {
        if (r <= ranges [card])
        {
            break;
        }
    }
    hands [player] [slot] = card;

    /* Animate */
    if (player_visible [player] == false)
    {
        card = CARD_BACK;
    }

    card_slide_from (DRAW_X_SPRITE, DRAW_Y_SPRITE, card, slot);
    if (fast)
    {
        card_slide_to_fast (slot << 5, HAND_Y_SPRITE);
    }
    else
    {
        card_slide_to (slot << 5, HAND_Y_SPRITE);
    }
    render_card_as_background (slot << 2, HAND_Y_TILE, card, slot);
    card_slide_done ();
}


/*
 * Add a resource to a player.
 * A bound of 999 is used as this is what we can display.
 */
static void resource_add (uint16_t *resource, uint16_t count)
{
    *resource += count;
    if (*resource > 999)
    {
        *resource = 999;
    }
}


/*
 * Subtract a resource from a player, enforcing a lower bound of zero.
 */
static void resource_subtract (uint16_t *resource, uint16_t count)
{
    if (*resource > count)
    {
        *resource -= count;
    }
    else
    {
        *resource = 0;
    }
}


/*
 * Attack the enemy.
 */
static void attack_enemy (uint16_t count)
{
    uint8_t enemy = !player;

    /* The fence takes damage first */
    if (resources [enemy] [FENCE] >= count)
    {
        resources [enemy] [FENCE] -= count;
        castle_damaged = false;
        return;
    }

    /* The castle takes damage once the fence has been destroyed */
    count -= resources [enemy] [FENCE];
    resources [enemy] [FENCE] = 0;
    resource_subtract (&resources [enemy] [CASTLE], count);
    castle_damaged = true;
}


/*
 * Play a card from the active player's hand.
 */
static void play_card (uint8_t slot)
{
    card_t card = hands [player] [slot];
    hands [player] [slot] = CARD_NONE;
    empty_slot = slot;

    /* Subtract cost */
    uint8_t cost_unit = card_data [card].cost_unit;
    uint8_t card_cost = card_data [card].cost;
    resources [player] [cost_unit] -= card_cost;
    panel_update ();

    /* Animate */
    card_slide_from (slot << 5, HAND_Y_SPRITE, card, slot);
    render_card_as_background (slot << 2, HAND_Y_TILE, CARD_NONE, slot);
    card_slide_to (DISCARD_X_SPRITE, DISCARD_Y_SPRITE);
    render_card_as_background (DISCARD_X_TILE, DISCARD_Y_TILE, card, 8);
    card_slide_done ();

    uint8_t enemy = !player;

    switch (card)
    {
        case CARD_WALL:
            resource_add (&resources [player] [FENCE], 3);
            break;

        case CARD_BASE:
            resource_add (&resources [player] [CASTLE], 2);
            break;

        case CARD_DEFENCE:
            resource_add (&resources [player] [FENCE], 6);
            break;

        case CARD_RESERVE:
            resource_add (&resources [player] [CASTLE], 8);
            resource_subtract (&resources [player] [FENCE], 4);
            break;

        case CARD_TOWER:
            resource_add (&resources [player] [CASTLE], 5);
            break;

        case CARD_SCHOOL:
            resource_add (&resources [player] [BUILDERS], 1);
            break;

        case CARD_WAIN:
            resource_add (&resources [player] [CASTLE], 8);
            resource_subtract (&resources [enemy] [CASTLE], 4);
            break;

        case CARD_FENCE:
            resource_add (&resources [player] [FENCE], 22);
            break;

        case CARD_FORT:
            resource_add (&resources [player] [CASTLE], 20);
            break;

        case CARD_BABYLON:
            resource_add (&resources [player] [CASTLE], 32);
            break;

        case CARD_ARCHER:
            attack_enemy (2);
            break;

        case CARD_KNIGHT:
            attack_enemy (3);
            break;

        case CARD_RIDER:
            attack_enemy (4);
            break;

        case CARD_PLATOON:
            attack_enemy (6);
            break;

        case CARD_RECRUIT:
            resource_add (&resources [player] [SOLDIERS], 1);
            break;

        case CARD_ATTACK:
            attack_enemy (12);
            break;

        case CARD_SABOTEUR:
            resource_subtract (&resources [enemy] [BRICKS], 4);
            resource_subtract (&resources [enemy] [WEAPONS], 4);
            resource_subtract (&resources [enemy] [CRYSTALS], 4);
            break;

        case CARD_THIEF:
            resource_add (&resources [player] [BRICKS],   MIN (resources [enemy] [BRICKS], 5));
            resource_add (&resources [player] [WEAPONS],  MIN (resources [enemy] [WEAPONS], 5));
            resource_add (&resources [player] [CRYSTALS], MIN (resources [enemy] [CRYSTALS], 5));
            resource_subtract (&resources [enemy] [BRICKS], 5);
            resource_subtract (&resources [enemy] [WEAPONS], 5);
            resource_subtract (&resources [enemy] [CRYSTALS], 5);
            break;

        case CARD_SWAT:
            resource_subtract (&resources [enemy] [CASTLE], 10);
            break;

        case CARD_BANSHEE:
            attack_enemy (32);
            break;

        case CARD_CONJURE_BRICKS:
            resource_add (&resources [player] [BRICKS], 8);
            break;

        case CARD_CRUSH_BRICKS:
            resource_subtract (&resources [enemy] [BRICKS], 8);
            break;

        case CARD_CONJURE_WEAPONS:
            resource_add (&resources [player] [WEAPONS], 8);
            break;

        case CARD_CRUSH_WEAPONS:
            resource_subtract (&resources [enemy] [WEAPONS], 8);
            break;

        case CARD_CONJURE_CRYSTALS:
            resource_add (&resources [player] [CRYSTALS], 8);
            break;

        case CARD_CRUSH_CRYSTALS:
            resource_subtract (&resources [enemy] [CRYSTALS], 8);
            break;

        case CARD_SORCERER:
            resource_add (&resources [player] [MAGI], 1);
            break;

        case CARD_DRAGON:
            attack_enemy (25);
            break;

        case CARD_PIXIES:
            resource_add (&resources [player] [CASTLE], 22);
            break;

        case CARD_CURSE:
            for (uint8_t resource = 0; resource < FIELD_MAX; resource++)
            {
                resource_add (&resources [player] [resource], 1);
                resource_subtract (&resources [enemy] [resource], 1);

                /* Ensure the enemy has at least one of each production type */
                if (resource == BUILDERS || resource == SOLDIERS || resource == MAGI)
                {
                    if (resources [enemy] [resource] < 1)
                    {
                        resources [enemy] [resource] = 1;
                    }
                }
            }
            break;

        default:
            break;
    }

    castle_update ();
    fence_update ();
    panel_update ();

    switch (card)
    {
        case CARD_BASE:
        case CARD_RESERVE:
        case CARD_TOWER:
        case CARD_WAIN:
        case CARD_FORT:
        case CARD_BABYLON:
        case CARD_PIXIES:
            play_build_castle_sound ();
            break;

        case CARD_SWAT:
            play_ruin_castle_sound ();
            break;

        case CARD_WALL:
        case CARD_DEFENCE:
        case CARD_FENCE:
            play_build_fence_sound ();
            break;

        case CARD_SCHOOL:
        case CARD_RECRUIT:
        case CARD_SORCERER:
            play_increase_power_sound ();
            break;

        case CARD_CONJURE_BRICKS:
        case CARD_CONJURE_WEAPONS:
        case CARD_CONJURE_CRYSTALS:
        case CARD_THIEF:
            play_increase_stocks_sound ();
            break;

        case CARD_CRUSH_BRICKS:
        case CARD_CRUSH_WEAPONS:
        case CARD_CRUSH_CRYSTALS:
        case CARD_SABOTEUR:
            play_decrease_stocks_sound ();
            break;

        case CARD_ARCHER:
        case CARD_KNIGHT:
        case CARD_RIDER:
        case CARD_PLATOON:
        case CARD_ATTACK:
        case CARD_BANSHEE:
        case CARD_DRAGON:
            if (castle_damaged)
            {
                play_ruin_castle_sound ();
            }
            else
            {
                play_ruin_fence_sound ();
            }
            break;

        case CARD_CURSE:
            play_curse_sound ();
            break;

        default:
            break;
    }
}


/*
 * Discard a card from the active player's hand.
 */
static void discard_card (uint8_t slot)
{
    card_t card = hands [player] [slot];
    hands [player] [slot] = CARD_NONE;
    empty_slot = slot;

    /* Animate */
    card_slide_from (slot << 5, HAND_Y_SPRITE, card | DISCARD_BIT, slot);
    render_card_as_background (slot << 2, HAND_Y_TILE, CARD_NONE, slot);
    card_slide_to (DISCARD_X_SPRITE, DISCARD_Y_SPRITE);
    render_card_as_background (DISCARD_X_TILE, DISCARD_Y_TILE, card | DISCARD_BIT, 8);
    card_slide_done ();
}


/*
 * Slide a card off-screen, used at the end of a game.
 */
static void clear_card (uint8_t slot)
{
    card_t card = hands [player] [slot];
    hands [player] [slot] = CARD_NONE;

    if (!player_visible [player])
    {
        card = CARD_BACK;
    }

    /* Animate */
    card_slide_from (slot << 5, HAND_Y_SPRITE, card, slot);
    render_card_as_background (slot << 2, HAND_Y_TILE, CARD_NONE, slot);
    card_slide_to_fast (slot << 5, 192);
    card_slide_done ();
}


/*
 * Change the current player.
 */
static void set_player (uint8_t p)
{
    player = p;
    card_t *hand = hands [player];

    /* Player indicator */
    panel_update_player (p);

    /* Show the new player's hand */
    for (uint8_t slot = 0; slot < 8; slot++)
    {
        if (hand [slot] == CARD_NONE)
        {
            render_card_as_background (slot << 2, HAND_Y_TILE, CARD_NONE, slot);
        }
        else if (player_visible [player])
        {
            render_card_as_background (slot << 2, HAND_Y_TILE, hand [slot], slot);
        }
        else
        {
            render_card_as_background (slot << 2, HAND_Y_TILE, CARD_BACK, slot);
        }
    }
}


/*
 * Generate resources for the current player.
 */
static void generate_resources (void)
{
    resource_add (&resources [player] [BRICKS],   resources [player] [BUILDERS]);
    resource_add (&resources [player] [WEAPONS],  resources [player] [SOLDIERS]);
    resource_add (&resources [player] [CRYSTALS], resources [player] [MAGI]);

    panel_update ();
}


/*
 * Check if the player can afford to play a card.
 */
static bool card_valid (card_t card)
{
    uint8_t cost_unit = card_data [card].cost_unit;
    uint8_t card_cost = card_data [card].cost;

    return (resources [player] [cost_unit] >= card_cost);
}


/*
 * Draw the gameplay cursor.
 */
static void draw_cursor (uint8_t x)
{
    card_t card = hands [player] [x];
    x = (x << 5) + 8;

    /* To save some VRAM pattern memory, only
     * keep one cursor loaded at a time. */

    /* The cursors are in bank 2 */
    SMS_mapROMBank (2);
    if (card_valid (card))
    {
        SMS_loadTiles (&cursor_patterns [32], PATTERN_HAND_CURSOR, 128);
    }
    else
    {
        SMS_loadTiles (&cursor_patterns [64], PATTERN_HAND_CURSOR, 128);
    }
    SMS_mapROMBank (3);


    SMS_initSprites ();

    /* Cursor patterns are loaded at the base of vram */
    SMS_addSprite (x,     172,     PATTERN_HAND_CURSOR    );
    SMS_addSprite (x + 8, 172,     PATTERN_HAND_CURSOR + 1);
    SMS_addSprite (x,     172 + 8, PATTERN_HAND_CURSOR + 2);
    SMS_addSprite (x + 8, 172 + 8, PATTERN_HAND_CURSOR + 3);

    SMS_copySpritestoSAT ();
}


/*
 * Clear the cursor.
 */
static void clear_cursor (void)
{
    SMS_initSprites ();
    SMS_copySpritestoSAT ();
}


/*
 * Take player input to select a card.
 */
static void human_move (void)
{
    /* Keep track of the two player cursor positions separately */
    static uint8_t cursor_position [2] = { 0, 0 };

    uint16_t player_mask;

    /* For 2-player mode, the reds use the 2nd controller port. */
    if (player_human [0] && player_human [1] && player == 1)
    {
        player_mask = PORT_B_KEY_LEFT | PORT_B_KEY_RIGHT | PORT_B_KEY_1 | PORT_B_KEY_2;
    }
    else
    {
        player_mask = PORT_A_KEY_LEFT | PORT_A_KEY_RIGHT | PORT_A_KEY_1 | PORT_A_KEY_2;
    }

    draw_cursor (cursor_position [player]);

    while (true)
    {
        uint16_t key_pressed = SMS_getKeysPressed () & player_mask;

        if (key_pressed & (PORT_A_KEY_LEFT | PORT_B_KEY_LEFT))
        {
            if (cursor_position [player] > 0)
            {
                cursor_position [player] -= 1;
            }
            draw_cursor (cursor_position [player]);
        }
        else if (key_pressed & (PORT_A_KEY_RIGHT | PORT_B_KEY_RIGHT))
        {
            if (cursor_position [player] < 7)
            {
                cursor_position [player] += 1;
            }
            draw_cursor (cursor_position [player]);
        }
        else if (key_pressed & (PORT_A_KEY_1 | PORT_B_KEY_1))
        {
            card_t card = hands [player] [cursor_position [player]];

            if (card_valid (card))
            {
                clear_cursor ();
                play_card (cursor_position [player]);
                return;
            }
        }
        else if (key_pressed & (PORT_A_KEY_2 | PORT_B_KEY_2))
        {
            clear_cursor ();
            discard_card (cursor_position [player]);
            return;
        }

        SMS_waitForVBlank ();

        if (reset)
        {
            return;
        }
    }
}


/*
 * Select a move for the current player.
 * If the card can be afforded, it is to be played.
 * If the card is too expensive, it is to be discarded.
 */
static void ai_move (void)
{
    uint8_t best_card = 0;
    uint8_t best_score = 0;

    /* Play the most expensive card, or increase production if possible */
    for (uint8_t slot = 0; slot < 8; slot++)
    {
        card_t card = hands [player] [slot];

        if (card_valid (card))
        {
            uint8_t test_score = card_data [card].cost;

            if (card == CARD_SCHOOL || card == CARD_RECRUIT || card == CARD_SORCERER)
            {
                test_score = 100;
            }

            if (test_score > best_score)
            {
                best_card = slot;
                best_score = test_score;
            }
        }
    }

    if (best_score)
    {
        play_card (best_card);
    }
    else
    {
        /* If there are no valid moves, discard the most expensive card */
        for (uint8_t slot = 0; slot < 8; slot++)
        {
            card_t card = hands [player] [slot];

            if (card_data [card].cost > best_score)
            {
                best_card = slot;
                best_score = card_data [card].cost;
            }
        }

        discard_card (best_card);
    }
}


/*
 * Fill the name table with tile-zero.
 */
static inline void clear_background (void)
{
    uint16_t blank_line [32] = { 0 };

    for (uint8_t row = 0; row < 24; row++)
    {
        SMS_loadTileMapArea (0, row, &blank_line, 32, 1);
    }
}


/*
 * Play one game of Ants.
 */
void game_start (void)
{
    /* Once-off setup before starting the game loop.
     * Only needed when entering the game after leaving the title menu. */
    SMS_displayOff ();
    SMS_loadBGPalette (background_palette);
    SMS_loadSpritePalette (sprite_palette);

    /* Setup for gameplay */
    SMS_loadTiles (blank_pattern, PATTERN_BLANK, sizeof (blank_pattern));
    clear_background ();

    /* Player indicators and panels are in bank 3 */
    SMS_mapROMBank (3);
    SMS_loadTiles (player_patterns, PATTERN_PLAYERS, sizeof (player_patterns));
    SMS_loadTiles (panel_patterns, PATTERN_PANELS, sizeof (panel_patterns));

    card_buffer_prepare ();

    /* Draw the background */
    SMS_loadTiles (background_patterns, PATTERN_BACKGROUND, sizeof (background_patterns));
    for (uint16_t i = 0; i < 432; i++)
    {
        vdp_background_indices [i] = background_indices [i] + PATTERN_BACKGROUND;
    }
    SMS_loadTileMapArea (4, 0, vdp_background_indices, 24, 18);

    /* Extend background sky to cover areas above panels */
    SMS_loadTileMapArea ( 0, 0, vdp_background_indices, 4, 1);
    SMS_loadTileMapArea (28, 0, vdp_background_indices, 4, 1);

    /* Extend background grass to cover areas below panels */
    uint16_t grass_left [4] = {
        vdp_background_indices [408], vdp_background_indices [408],
        vdp_background_indices [408], vdp_background_indices [408]
    };
    uint16_t grass_right [4] = {
        vdp_background_indices [431], vdp_background_indices [431],
        vdp_background_indices [431], vdp_background_indices [431]
    };
    SMS_loadTileMapArea ( 0, 17, grass_left, 4, 1);
    SMS_loadTileMapArea (28, 17, grass_right, 4, 1);

    /* Draw the draw deck and side panels */
    render_card_as_background (12, 0, CARD_BACK, 9);
    render_card_as_background (16, 0, CARD_NONE, 8);
    panel_init_wins ();
    panel_init ();

    /* Reset castle and fence value cache */
    castle_init ();
    fence_init ();

    /* Clear any sprites */
    SMS_initSprites ();
    SMS_copySpritestoSAT ();

    /* Draw initial values */
    memcpy (resources [0], starting_resources, sizeof (resources [0]));
    memcpy (resources [1], starting_resources, sizeof (resources [1]));
    panel_update ();
    castle_update ();
    fence_update ();
    SMS_displayOn ();

    /* Outer loop - Ensures that when one game is completed, another begins */
    while (!reset)
    {
        /* Set up for a new game */
        /* Clear hands and reset to starting resources */
        memset (hands [0], CARD_NONE, sizeof (hands [0]));
        memset (hands [1], CARD_NONE, sizeof (hands [1]));
        memcpy (resources [0], starting_resources, sizeof (resources [0]));
        memcpy (resources [1], starting_resources, sizeof (resources [1]));

        /* Draw the starting values into the panel, castle, and fence. */
        panel_update ();
        castle_update ();
        fence_update ();

        /* For a single-player game, the human always goes first.
         * Otherwise, the first player is randomised. */
        if (player_human [0] && !player_human [1])
        {
            set_player (0);
        }
        else if (player_human [1] && !player_human [0])
        {
            set_player (1);
        }
        else
        {
            set_player (rand () & 1);
        }

        /* Deal first player */
        for (uint8_t i = 0; i < 8; i++)
        {
            draw_card (i, true);
        }
        delay_frames (60);

        /* Deal second player */
        set_player (!player);
        for (uint8_t i = 0; i < 8; i++)
        {
            draw_card (i, true);
        }
        delay_frames (60);

        /* The player who starts does not generate
         * resources on their first turn. */
        bool first = true;

        /* Inner loop - Runs for one game */
        while (true)
        {
            /* Change player */
            set_player (!player);
            if (first)
            {
                /* Start the game with the increase-power sound */
                play_increase_power_sound ();
                first = false;
            }
            else
            {
                generate_resources ();
            }

            if (player_human [player])
            {
                human_move ();
            }
            else
            {
                delay_frames (30);
                ai_move ();
            }
            delay_frames (15);

            /* Check for win */
            if (infinite_game == false)
            {
                if (resources [player] [CASTLE] >= 100 || resources [!player] [CASTLE] == 0)
                {
                    wins [player] += 1;
                    panel_update_wins (player);

                    /* TODO: trumpet sprite */
                    play_fanfare_sound ();
                    break;
                }
            }

            /* Check for reset */
            if (reset)
            {
                break;
            }

            draw_card (empty_slot, false);
            delay_frames (30);
        }

        /* Tidy up after the completed game */
        for (uint8_t card = 0; card < 8; card++)
        {
            if (hands [player] [card] != CARD_NONE)
            {
                clear_card (card);
            }
        }
    }
}
