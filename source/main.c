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
void clear_background (void)
{
    uint16_t blank_line [32] = { 0 };

    for (uint8_t row = 0; row < 24; row++)
    {
        SMS_loadTileMapArea (0, row, &blank_line, 32, 1);
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
    SMS_loadTileMapArea (12, 0, panel_cards [30], 4, 6);
    SMS_loadTileMapArea (16, 0, panel_cards [0], 4, 6);

    uint16_t timer = 0;
    uint16_t x = 0;

    while (1)
    {
        SMS_waitForVBlank ();

        if (timer++ == 60)
        {
            timer = 0;
            SMS_loadTileMapArea (x, 18, panel_cards [rand () % 30], 4, 6);
            x = (x + 4) & 0x1f;
        }

    }
}

SMS_EMBED_SEGA_ROM_HEADER(9999, 0);
