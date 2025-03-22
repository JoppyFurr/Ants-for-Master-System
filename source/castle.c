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

        /* Name-table entries for the vertical strips */
        uint16_t strip_map [72];
        memset (strip_map, 0, sizeof (strip_map)); /* Initialise to empty tile */

        /* Pattern entries for this strip */
        uint8_t pattern_buffer [6] [32 * 5];
        memset (pattern_buffer, 0, sizeof (pattern_buffer)); /* Initialise to black */

        /* Note: Currently the castle is generated in vertical slices.
         *       This could probably be reworked to remove the column loop. */
        for (uint8_t col = 0; col < 6; col++)
        {
            uint16_t pattern_index = 0;
            uint8_t strip_map_index = col + 6 * (12 - tiles_high);

            /* Place the peak (2 or 3 tiles) */
            strip_map [strip_map_index] = 0x0800 | strip_vram_base + pattern_index++;
            strip_map_index += 6;
            strip_map [strip_map_index] = 0x0800 | strip_vram_base + pattern_index++;
            strip_map_index += 6;
            if (peak_start > 3)
            {
                strip_map [strip_map_index] = 0x0800 | strip_vram_base + pattern_index++;
                strip_map_index += 6;
            }

            uint8_t body_height_from_peak = (body_height < 5) ? body_height : ((body_height - 5) & 0x07);

            /* Place the repeating section */
            uint8_t remaining_body_height = body_height - (body_height_from_peak);
            if (remaining_body_height > 8)
            {
                while (remaining_body_height > 8)
                {
                    strip_map [strip_map_index] = 0x0800 | strip_vram_base + pattern_index;
                    strip_map_index += 6;
                    remaining_body_height -= 8;
                }
                pattern_index++;
            }

            /* Place the base */
            if (remaining_body_height)
            {
                strip_map [strip_map_index] = 0x0800 | strip_vram_base + pattern_index;
            }

            uint16_t buffer_index = peak_start << 2;

            /* Start with the peaks, note not all 8 pixels are used in the first pattern. */
            memcpy (&pattern_buffer [col] [buffer_index], &castles_patterns [(castles_panels [player] [0 + col] << 3) + 3], 20);
            buffer_index += 20;
            memcpy (&pattern_buffer [col] [buffer_index], &castles_patterns [(castles_panels [player] [6 + col] << 3)], 32);
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
                    memcpy (&pattern_buffer [col] [buffer_index], &castles_patterns [(castles_panels [player] [12 + col] << 3)], 32);
                    buffer_index += 32;
                    body_height_draw -= 8;
                }
                else
                {
                    memcpy (&pattern_buffer [col] [buffer_index], &castles_patterns [(castles_panels [player] [12 + col] << 3)], body_height_draw << 2);
                    buffer_index += body_height_draw << 2; /* TODO: Needed to feed in the background? */
                    body_height_draw = 0;
                }
            }

            /* Advance to the next strip */
            strip_vram_base += 5;
        }

        /* To show a complete transition from one castle height to another within a single
         * frame, we need to start at VBlank and finish all writing before the castle is
         * on-screen. The castle tiles begin at line 48, so finishing by then is the goal.
         *
         * For PAL consoles, the following code completes by line 23, so the full transition
         * is completed glitch-free each time.
         *
         * For NTSC consoles, the following code completes by line 74. So, the peaks of the
         * castle may glitch slightly for transitions greater than ~90 height.
         *
         * Possibly this could be improved by skipping unchanged parts of the tile-map, or
         * by using unsafe writes for data that we know will occur during VBlank. */
        SMS_waitForVBlank ();
        SMS_loadTileMapArea (col_base, 6, strip_map, 6, 12);
        SMS_loadTiles (pattern_buffer, (player == 0) ? PATTERN_CASTLE_1_BUFFER : PATTERN_CASTLE_2_BUFFER, sizeof (pattern_buffer));

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

        /* Write to VRAM */
        SMS_waitForVBlank ();
        SMS_loadTileMapArea ((player == 0) ? 13 : 18, 6, strip_map, 1, 12);
        SMS_loadTiles (pattern_buffer, strip_vram_base, sizeof (pattern_buffer));

        /* Update the cache */
        cache [player] = value;
    }
}
