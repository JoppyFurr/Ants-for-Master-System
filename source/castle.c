/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 *
 * Castle rendering
 */

#include <stdint.h>
#include <string.h>

#include "SMSlib.h"

#include "vram.h"
#include "game.h"

/* Pattern and index data */
extern const uint16_t castles_panels [2] [18];
extern const uint32_t castles_patterns [];

/* Game state */
extern uint16_t resources [2] [FIELD_MAX];

/*
 * Draw the castles.
 *
 * TODO:
 *  - Speed up rendering, only update what needs to be updated.
 *  - If needed, consider faster rendering by having the castle
 *    grow from the top instead of the bottom. This would mean
 *    that only the top needs to be re-drawn.
 *  - Coloured windows like in the original
 *  - Render onto a background image
 *
 * Notes:
 *  - Base of castle is at y = 140
 *  - Uses the sprite palette
 *  - Castle is built out of vertical strips. Each strip can use up to five patterns:
 *      - Pattern [0..2] contain the peak area of the castle
 *      - Pattern [   3] contains the repeatable body of the castle.
 *      - Pattern [   4] contains the base of the castle.
 *      - Shorter castles can use 2-4 patterns instead of all five.
 *      - We need to allocate 30 patterns per castle.
 */
void castle_update (void)
{
    static uint16_t cache [2] = { 0 };

    for (uint8_t player = 0; player < 2; player++)
    {
        uint16_t value = resources [player] [CASTLE];

        /* Only draw a castle up to height 100 */
        if (value > 100)
        {
            value = 100;
        }

        /* Only update if there has been a change */
        if (value == cache [player])
        {
            continue;
        }

        /* We don't have room for a 100-pixel high castle body.
         * So, use a scale of 100-castle:80-pixels. */
        uint8_t body_height = (value << 2) / 5;

        uint8_t tiles_high = 2 + ((body_height + 7) >> 3);
        uint8_t peak_start = (8 - (body_height & 7)) & 7;
        uint16_t strip_vram_base = (player == 0) ? PATTERN_CASTLE_1_BUFFER : PATTERN_CASTLE_2_BUFFER;
        uint8_t col_base = (player == 0) ? 6 : 20;

        for (uint8_t col = 0; col < 6; col++)
        {
            /* Name-table entries for this strip */
            uint16_t strip_map [12] = { 0 };
            uint16_t pattern_index = 0;
            for (uint8_t row = 12 - tiles_high; row < 12; row++)
            {
                strip_map [row] = (pattern_index + strip_vram_base) | 0x0800;

                /* Repeat the castle body pattern */
                if (pattern_index == 3 && row < 10)
                    continue;

                pattern_index++;
            }
            SMS_loadTileMapArea (col_base + col, 6, strip_map, 1, 12);

            /* Pattern entries for this strip */
            uint8_t pattern_buffer [32 * 5] = { 0 };
            uint16_t buffer_index = peak_start << 2;

            /* Start with the peaks, note not all 8 pixels are used in the first pattern. */
            memcpy (&pattern_buffer [buffer_index], &castles_patterns [(castles_panels [player] [0 + col] << 3) + 3], 20);
            buffer_index += 20;
            memcpy (&pattern_buffer [buffer_index], &castles_patterns [(castles_panels [player] [6 + col] << 3)], 32);
            buffer_index += 32;

            /* Account for the repeated section */
            uint8_t body_height_draw = body_height;
            while (body_height_draw > 24)
            {
                body_height_draw -= 8;
            }

            /* Draw the castle-body pattern */
            while (body_height_draw)
            {
                if (body_height_draw > 8)
                {
                    memcpy (&pattern_buffer [buffer_index], &castles_patterns [(castles_panels [player] [12 + col] << 3)], 32);
                    buffer_index += 32;
                    body_height_draw -= 8;
                }
                else
                {
                    memcpy (&pattern_buffer [buffer_index], &castles_patterns [(castles_panels [player] [12 + col] << 3)], body_height_draw << 2);
                    buffer_index += body_height_draw << 2; /* TODO: Needed to feed in the background? */
                    body_height_draw = 0;
                }
            }

            /* For now, always write the full five patterns of the strip. */
            SMS_loadTiles (pattern_buffer, strip_vram_base, 160);

            /* Advance to the next strip */
            strip_vram_base += 5;
        }

        /* Update the cache */
        cache [0] = value;
    }
}


