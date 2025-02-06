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

extern const uint16_t fence_panels [2] [2];
extern const uint32_t fence_patterns [];

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
 *      - Two or three patterns contain the peak area of the castle
 *      - One contains the repeatable body of the castle.
 *      - One pattern contains the base of the castle.
 *      - Shorter castles may not need the repeating section or base section.
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

            uint8_t strip_map_index = (12 - tiles_high);

            /* Place the peak (2 or 3 tiles) */
            strip_map [strip_map_index++] = 0x0800 | strip_vram_base + pattern_index++;
            strip_map [strip_map_index++] = 0x0800 | strip_vram_base + pattern_index++;
            if (peak_start > 3)
            {
                strip_map [strip_map_index++] = 0x0800 | strip_vram_base + pattern_index++;
            }

            uint8_t body_height_from_peak = (body_height < 5) ? body_height : ((body_height - 5) & 0x07);

            /* Place the repeating section */
            uint8_t remaining_body_height = body_height - (body_height_from_peak);
            if (remaining_body_height > 8)
            {
                while (remaining_body_height > 8)
                {
                    strip_map [strip_map_index++] = 0x0800 | strip_vram_base + pattern_index;
                    remaining_body_height -= 8;
                }
                pattern_index++;
            }

            /* Place the base */
            if (remaining_body_height)
            {
                strip_map [strip_map_index] = 0x0800 | strip_vram_base + pattern_index;
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

            /* Account for the repeated section. The base contains up to five height,
             * so by height 21 there are two body-tiles that can share a pattern. */
            uint8_t body_height_draw = body_height;
            while (body_height_draw > 20)
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
            SMS_loadTiles (pattern_buffer, strip_vram_base, sizeof (pattern_buffer));

            /* Advance to the next strip */
            strip_vram_base += 5;
        }

        /* Update the cache */
        cache [player] = value;
    }
}


/*
 * Draw the fences.
 *
 * Notes:
 *  - Base of fence is at y = 140
 *  - Uses the sprite palette
 *  - The fence can use up to four patterns:
 *      - One or two patterns contain the white top area of the fence
 *      - One contains the repeatable body of the fence.
 *      - One pattern contains the base of the fence.
 *      - Shorter fences may not need the repeating section or base section.
 */
void fence_update (void)
{
    static uint16_t cache [2] = { 0 };

    for (uint8_t player = 0; player < 2; player++)
    {
        uint16_t value = resources [player] [FENCE];

        /* Only draw a fence up to height 100 */
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

        uint8_t tiles_high = 2 + ((body_height + 1) >> 3);
        uint8_t peak_start = (14 - (body_height & 7)) & 7;
        uint16_t strip_vram_base = (player == 0) ? PATTERN_FENCE_1_BUFFER : PATTERN_FENCE_2_BUFFER;
        uint8_t tiles_x = (player == 0) ? 13 : 18;

        /* Name-table entries for this strip */
        uint16_t strip_map [12] = { 0 };
        uint16_t pattern_index = 0;

        uint8_t strip_map_index = (12 - tiles_high);

        /* Place the peak (1 or 2 tiles) */
        strip_map [strip_map_index++] = 0x0800 | strip_vram_base + pattern_index++;
        if (peak_start > 1)
        {
            strip_map [strip_map_index++] = 0x0800 | strip_vram_base + pattern_index++;
        }

        uint8_t body_height_from_peak = (body_height < 5) ? body_height : ((body_height - 5) & 0x07);

        /* Place the repeating section */
        uint8_t remaining_body_height = body_height - (body_height_from_peak);
        if (remaining_body_height > 8)
        {
            while (remaining_body_height > 8)
            {
                strip_map [strip_map_index++] = 0x0800 | strip_vram_base + pattern_index;
                remaining_body_height -= 8;
            }
            pattern_index++;
        }

        /* Place the base */
        if (remaining_body_height)
        {
            strip_map [strip_map_index] = 0x0800 | strip_vram_base + pattern_index;
        }

        SMS_loadTileMapArea (tiles_x, 6, strip_map, 1, 12);

        /* Pattern entries for this strip */
        uint8_t pattern_buffer [32 * 4] = { 0 };
        uint16_t buffer_index = peak_start << 2;

        /* Draw the white top of the fence, note that this is fewer than 8 pixels high. */
        memcpy (&pattern_buffer [buffer_index], &fence_patterns [(fence_panels [player] [0] << 3) + 1], 28);
        buffer_index += 28;

        /* Account for the repeated section. The base contains up to five height,
         * so by height 21 there are two body-tiles that can share a pattern. */
        uint8_t body_height_draw = body_height;
        while (body_height_draw > 20)
        {
            body_height_draw -= 8;
        }

        /* Draw the fence-body pattern */
        while (body_height_draw)
        {
            if (body_height_draw > 8)
            {
                memcpy (&pattern_buffer [buffer_index], &fence_patterns [(fence_panels [player] [1] << 3)], 32);
                buffer_index += 32;
                body_height_draw -= 8;
            }
            else
            {
                memcpy (&pattern_buffer [buffer_index], &fence_patterns [(fence_panels [player] [1] << 3)], body_height_draw << 2);
                buffer_index += body_height_draw << 2; /* TODO: Needed to feed in the background? */
                body_height_draw = 0;
            }
        }

        /* For now, always write the full five patterns of the strip. */
        SMS_loadTiles (pattern_buffer, strip_vram_base, sizeof (pattern_buffer));

        /* Update the cache */
        cache [player] = value;
    }
}
