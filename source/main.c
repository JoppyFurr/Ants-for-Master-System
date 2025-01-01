/*
 * Ants for Master System
 *
 * An Ants clone for the Sega Master System
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "SMSlib.h"

#define TARGET_SMS
#include "../tile_data/palette.h"
#include "../tile_data/patterns.h"
#include "../tile_data/pattern_index.h"

#include "cards.h"
#include "save.h"
#include "rng.h"

#define HAND_Y_TILE 18
#define HAND_Y_SPRITE 144

#define DRAW_X_TILE 12
#define DRAW_Y_TILE 0
#define DRAW_X_SPRITE 96
#define DRAW_Y_SPRITE 0

#define DISCARD_X_TILE 16
#define DISCARD_Y_TILE 0
#define DISCARD_X_SPRITE 128
#define DISCARD_Y_SPRITE 0

/* Hands */
card_t hand_1 [8] = { 0 };

/* Slide animation variables */
uint16_t slide_start_x = 0;
uint16_t slide_start_y = 0;
card_t slide_card = 0;


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
 * Render a card as sprites.
 */
static inline void render_card_as_sprite (uint16_t x, uint16_t y, card_t card)
{
    uint8_t pattern_index = 0;

    for (uint8_t panel_y = 0; panel_y < 6; panel_y++)
    for (uint8_t panel_x = 0; panel_x < 4; panel_x++)
    {
        /* TODO: The sprite attribute table only allows an 8-bit pattern index.
         *       The cards use more than 256 patterns, so the particular card
         *       we want to render may need to be copied to a lower VRAM address. */
        SMS_addSprite (x + (panel_x << 3),
                       y + (panel_y << 3), panel_cards [card] [pattern_index++]);

    }
}


/*
 * Clear a card from the background.
 */
static inline void clear_card_from_tile (uint8_t x, uint8_t y)
{
    uint16_t empty_slot [24] = {
        PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY,
        PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY,
        PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY,
        PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY,
        PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY,
        PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY
    };
    SMS_loadTileMapArea (x, y, empty_slot, 4, 6);
}


/*
 * Render a card to the background.
 */
static inline void render_card_as_tile (uint8_t x, uint8_t y, card_t card)
{
    SMS_loadTileMapArea (x, y, panel_cards [card], 4, 6);
}


/*
 * Prepare for the card sliding animation.
 * This will render the card as a sprite in the starting position.
 */

void card_slide_from (uint16_t start_x, uint16_t start_y, card_t card)
{
    slide_start_x = start_x;
    slide_start_y = start_y;

    SMS_initSprites ();
    render_card_as_sprite (start_x, start_y, card);

    SMS_waitForVBlank ();
    SMS_copySpritestoSAT ();
}


/*
 * Animate a card sliding from the start position to the end
 * position, leaving it rendered as a sprite.
 */
void card_slide_to (uint16_t end_x, uint16_t end_y, card_t card)
{
    uint16_t x;
    uint16_t y;

    x = slide_start_x << 4;
    y = slide_start_y << 4;

    for (uint8_t frame = 0; frame < 16; frame++)
    {
        x += end_x - slide_start_x;
        y += end_y - slide_start_y;

        SMS_initSprites ();
        render_card_as_sprite (x >> 4, y >> 4, card);

        /* Write the new sprite position only during vblank. */
        SMS_waitForVBlank ();
        SMS_copySpritestoSAT ();
    }

    /* Note, we don't clear the sprite here. First the
     * caller needs to draw the card into the background. */
}


/*
 * Clear the card sprite animation. This should be called
 * once the card has been drawn to the background.
 */
void card_slide_done (void)
{
    SMS_initSprites ();
    SMS_copySpritestoSAT ();
}


/*
 * Draw a card and place it into the active player's hand.
 */
void draw_card (uint8_t slot)
{
    /* Deal */
    card_t card = rand () % 30;
    hand_1 [slot] = card;

    /* Animate */
    card_slide_from (DRAW_X_SPRITE, DRAW_Y_SPRITE, card);
    card_slide_to (slot << 5, HAND_Y_SPRITE, card);
    render_card_as_tile (slot << 2, HAND_Y_TILE, card);
    card_slide_done ();
}


/*
 * Play a card from the active player's hand.
 */
void play_card (uint8_t slot)
{
    card_t card = hand_1 [slot];

    /* Animate */
    card_slide_from (slot << 5, HAND_Y_SPRITE, card);
    clear_card_from_tile (slot << 2, HAND_Y_TILE);
    card_slide_to (DISCARD_X_SPRITE, DISCARD_Y_SPRITE, card);
    render_card_as_tile (DISCARD_X_TILE, DISCARD_Y_TILE, card);
    card_slide_done ();
}


/*
 * Delay until the chosen number of frames have passed.
 */
void delay_frames (uint8_t frames)
{
    while (frames)
    {
        SMS_waitForVBlank ();
        frames--;
    }
}


/*
 * Entry point.
 */
void main (void)
{
    /* Setup */
    SMS_loadBGPalette (palette);
    SMS_loadSpritePalette (palette);
    SMS_setBackdropColor (0);

    SMS_loadTiles (patterns, 0, sizeof (patterns));
    clear_background ();

    SMS_useFirstHalfTilesforSprites (true);
    SMS_initSprites ();
    SMS_copySpritestoSAT ();

    SMS_displayOn ();

    sram_load ();
    rng_seed ();

    /* Draw / discard area */
    render_card_as_tile (12, 0, CARD_BACK);
    render_card_as_tile (16, 0, CARD_RESERVE);

    /* Deal initial hand */
    for (uint8_t i = 0; i < 8; i++)
    {
        draw_card (i);
    }

    while (1)
    {
        uint8_t move = rand () & 0x07;

        delay_frames (60);
        play_card (move);

        delay_frames (30);
        draw_card (move);
    }
}

SMS_EMBED_SEGA_ROM_HEADER(9999, 0);
