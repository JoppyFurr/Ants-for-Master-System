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

#include "cards.h"
#include "game.h"

/* External data */
extern uint16_t panel_player [4] [8];

/* External functions */
extern void card_slide_from (uint16_t start_x, uint16_t start_y, card_t card);
extern void card_slide_to (uint16_t end_x, uint16_t end_y);
extern void card_slide_done (void);
extern void render_card_as_tile (uint8_t x, uint8_t y, card_t card);
extern void panel_init (void);
extern void panel_update (field_t field);
extern void delay_frames (uint8_t frames);

/* Game State */
uint8_t player = 0;
card_t hands [2] [8];
uint16_t resources [FIELD_MAX];

/* Starting resources */
const uint16_t starting_resources [8] = {
    [P1_BUILDERS] = 2,
    [P1_BRICKS]   = 5,
    [P1_SOLDIERS] = 2,
    [P1_WEAPONS]  = 5,
    [P1_MAGI]     = 2,
    [P1_CRYSTALS] = 5,
    [P1_CASTLE]   = 30,
    [P1_FENCE]    = 10
};


/*
 * Draw a card and place it into the active player's hand.
 */
void draw_card (uint8_t slot, bool hidden)
{
    /* Deal */
    card_t card = rand () % 30;
    hands [player] [slot] = card;

    /* Animate */
    if (hidden)
    {
        card = CARD_BACK;
    }

    card_slide_from (DRAW_X_SPRITE, DRAW_Y_SPRITE, card);
    card_slide_to (slot << 5, HAND_Y_SPRITE);
    render_card_as_tile (slot << 2, HAND_Y_TILE, card);
    card_slide_done ();
}


/*
 * Play a card from the active player's hand.
 */
void play_card (uint8_t slot)
{
    card_t card = hands [player] [slot];

    /* Animate */
    card_slide_from (slot << 5, HAND_Y_SPRITE, card);
    render_card_as_tile (slot << 2, HAND_Y_TILE, CARD_NONE);
    card_slide_to (DISCARD_X_SPRITE, DISCARD_Y_SPRITE);
    render_card_as_tile (DISCARD_X_TILE, DISCARD_Y_TILE, card);
    card_slide_done ();
}


/*
 * Change the current player.
 */
void set_player (uint8_t p)
{
    player = p;
    card_t *hand = hands [player];

    /* Player indicator */
    SMS_loadTileMapArea ( 0, 1, panel_player [0 | (player == 0 ? 2 : 0)], 4, 2);
    SMS_loadTileMapArea (28, 1, panel_player [1 | (player == 1 ? 2 : 0)], 4, 2);

    /* Show the new player's hand */
    for (uint8_t slot = 0; slot < 8; slot++)
    {
        if (hand [slot] == CARD_NONE)
        {
            render_card_as_tile (slot << 2, HAND_Y_TILE, CARD_NONE);
        }
        else if (player == 0)
        {
            render_card_as_tile (slot << 2, HAND_Y_TILE, hand [slot]);
        }
        else
        {
            render_card_as_tile (slot << 2, HAND_Y_TILE, CARD_BACK);
        }
    }
}


/*
 * Generate resources for the current player.
 */
void generate_resources (void)
{
    if (player == 0)
    {
        resources [P1_BRICKS]   += resources [P1_BUILDERS];
        resources [P1_WEAPONS]  += resources [P1_SOLDIERS];
        resources [P1_CRYSTALS] += resources [P1_MAGI];
        panel_update (P1_BRICKS);
        panel_update (P1_WEAPONS);
        panel_update (P1_CRYSTALS);
    }
    else
    {
        resources [P2_BRICKS]   += resources [P2_BUILDERS];
        resources [P2_WEAPONS]  += resources [P2_SOLDIERS];
        resources [P2_CRYSTALS] += resources [P2_MAGI];
        panel_update (P2_BRICKS);
        panel_update (P2_WEAPONS);
        panel_update (P2_CRYSTALS);
    }
}


/*
 * Play one game of Ants.
 */
void game_start (void)
{
    /* Clear hands and reset to starting resources */
    memset (hands [0], CARD_NONE, sizeof (hands [0]));
    memset (hands [1], CARD_NONE, sizeof (hands [1]));
    memcpy (&resources [0], starting_resources, sizeof (starting_resources));
    memcpy (&resources [8], starting_resources, sizeof (starting_resources));

    /* Draw side panels */
    panel_init ();
    for (uint8_t field = 0; field < FIELD_MAX; field++)
    {
        panel_update (field);
    }

    /* Draw / discard area */
    render_card_as_tile (12, 0, CARD_BACK);
    render_card_as_tile (16, 0, CARD_NONE);

    /* Draw player indicator */
    SMS_loadTileMapArea (0, 1, panel_player [2], 4, 2);
    SMS_loadTileMapArea (28, 1, panel_player [1], 4, 2);

    /* Deal Player 1 */
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
    bool generate = false;

    while (1)
    {
        /* Change player */
        set_player (!player);
        if (generate)
        {
            generate_resources ();
        }
        delay_frames (60);

        uint8_t move = rand () & 0x07;
        play_card (move);
        delay_frames (30);

        draw_card (move, (player == 1));
        delay_frames (30);

        generate = true;
    }

}
