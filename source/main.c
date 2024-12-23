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
 * Render a card to the background.
 */
static inline void render_card_as_tile (uint8_t x, uint8_t y, card_t card)
{
    SMS_loadTileMapArea (x, y, panel_cards [card], 4, 6);
}


/*
 * Animate a card sliding from one position to another.
 */
void card_slide (uint16_t start_x, uint16_t start_y,
                 uint16_t end_x, uint16_t end_y, card_t card)
{
    uint16_t x;
    uint16_t y;

    x = start_x << 4;
    y = start_y << 4;

    SMS_initSprites ();
    render_card_as_sprite (start_x, start_y, card);

    SMS_waitForVBlank ();
    SMS_copySpritestoSAT ();

    for (uint8_t frame = 0; frame < 16; frame++)
    {
        x += end_x - start_x;
        y += end_y - start_y;

        SMS_initSprites ();
        render_card_as_sprite (x >> 4, y >> 4, card);

        /* Write the new sprite position only during vblank. */
        SMS_waitForVBlank ();
        SMS_copySpritestoSAT ();
    }

    /* Note, we don't clear the sprite here, as first the caller
     * needs to place the card the background. */
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

    uint16_t timer = 0;
    uint16_t x = 0;

    while (1)
    {
        SMS_waitForVBlank ();

        if (timer++ == 60)
        {
            card_t card = rand () % 30;
            timer = 0;
            card_slide (96, 0, x << 3, 18 << 3, card);
            render_card_as_tile (x, 18, card);
            SMS_initSprites ();
            SMS_copySpritestoSAT ();
            x = (x + 4) & 0x1f;
        }

    }
}

SMS_EMBED_SEGA_ROM_HEADER(9999, 0);
