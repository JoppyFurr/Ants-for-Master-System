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

#define INCLUDE_CARD_DATA
#include "cards.h"
#include "castle.h"
#include "panel.h"
#include "game.h"
#include "sound.h"

#define MIN(X,Y) (((X) < (Y)) ? X : Y)

/* External functions */
extern void card_slide_from (uint16_t start_x, uint16_t start_y, card_t card, uint8_t slot);
extern void card_slide_to (uint16_t end_x, uint16_t end_y);
extern void card_slide_done (void);
extern void render_card_as_background (uint8_t x, uint8_t y, card_t card, uint8_t slot);
extern void delay_frames (uint8_t frames);

/* Game State */
uint8_t player = 0;
card_t hands [2] [8];
uint16_t resources [2] [FIELD_MAX];
uint8_t empty_slot = 0;
bool castle_damaged = false;

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
static void draw_card (uint8_t slot, bool hidden)
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
    if (hidden)
    {
        card = CARD_BACK;
    }

    card_slide_from (DRAW_X_SPRITE, DRAW_Y_SPRITE, card, slot);
    card_slide_to (slot << 5, HAND_Y_SPRITE);
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

            /* TODO */
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
 * TODO: Slower animation
 */
static void clear_card (uint8_t slot)
{
    card_t card = hands [player] [slot];
    hands [player] [slot] = CARD_NONE;

    /* Animate */
    card_slide_from (slot << 5, HAND_Y_SPRITE, card, slot);
    render_card_as_background (slot << 2, HAND_Y_TILE, CARD_NONE, slot);
    card_slide_to (slot << 5, 192);
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
        else if (player == 0)
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
 * Play one game of Ants.
 */
void game_start (void)
{
    /* Draw the draw deck and side panels */
    render_card_as_background (12, 0, CARD_BACK, 9);
    render_card_as_background (16, 0, CARD_NONE, 8);
    panel_init ();

    /* Outer loop - Ensures that when one game is completed, another begins */
    /* TODO - Starting player:
     *   -> For a 1-player game, the human is always the first player.
     *   -> Otherwise, the first player is randomised.
     */
    while (true)
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

        /* Draw player indicator */
        panel_update_player (0);

        /* Deal Player 1 */
        /* TODO: Fast-draw cards during deal? */
        set_player (0);
        for (uint8_t i = 0; i < 8; i++)
        {
            draw_card (i, false);
        }
        delay_frames (60);

        /* Deal Player 2 */
        set_player (1);
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
            delay_frames (60);

            /* For now both players are computers */
            ai_move ();
            delay_frames (30);

            /* Check for win */
            if (resources [player] [CASTLE] >= 100 || resources [!player] [CASTLE] == 0)
            {
                /* TODO: Play trumpet sound */
                break;
            }

            draw_card (empty_slot, (player == 1));
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
