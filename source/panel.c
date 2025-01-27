/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 *
 * Side-panel implementation
 */
#include <stdint.h>
#include <string.h>

#include "SMSlib.h"

#include "vram.h"
#include "digits.h"
#include "game.h"

/* Pattern and index data */
extern const uint16_t panel_indices [];
extern const uint32_t panel_patterns [];
extern uint16_t panel_patterns_start;

/* Game state */
extern uint16_t resources [2] [FIELD_MAX];

/*
 * Initialise the side-panel.
 */
void panel_init (void)
{
    const uint8_t y_positions [8] = { 4, 5, 7, 8, 10, 11, 14, 15 };

    uint16_t vdp_panel_indices [56];

    for (uint8_t i = 0; i < 56; i++)
    {
        vdp_panel_indices [i] = panel_indices [i] + panel_patterns_start;
    }

    /* Copy tilemap from image */
    SMS_loadTileMapArea (0, 3, vdp_panel_indices, 4, 14);
    SMS_loadTileMapArea (28, 3, vdp_panel_indices, 4, 14);

    /* Set up panel digit areas */
    for (uint8_t i = 0; i < 16; i++)
    {
        /* Eight digit areas for each player */
        uint8_t x = (i < 8) ? 2 : 30;
        uint8_t y = y_positions [i & 0x07];

        uint16_t area [2] = { PATTERN_PANEL_DIGITS + (i << 1), PATTERN_PANEL_DIGITS + (i << 1) + 1 };
        SMS_loadTileMapArea (x, y, area, 2, 1);
    }
}


/*
 * Update the side-panel values.
 */
void panel_update (void)
{
    static uint16_t cache [2] [FIELD_MAX] = { { 0 } };
    const uint8_t field_backgrounds [8] = { 6, 10, 18, 22, 30, 34, 46, 50 };

    /* A pair of buffers to hold the patterns we will draw into */
    uint8_t buffer_l [32];
    uint8_t buffer_r [32];


    for (uint8_t player = 0; player < 2; player++)
    for (uint8_t field = 0; field < FIELD_MAX; field++)
    {
        const uint16_t value = resources [player] [field];

        /* Skip fields that have not changed */
        if (value == cache [player] [field])
        {
            continue;
        }

        /* Separate the digits. Assume a maximum value of 999. */
        const uint8_t digit_0 = value % 10;           /* Ones */
        const uint8_t digit_1 = (value % 100) / 10;   /* Tens */
        const uint8_t digit_2 = value / 100;          /* Hundreds */

        /* Within each panel-box, the digit for the top
         * field is one pixel higher within its tile */
        const uint8_t y_offset = (field & 0x01) ? 2 : 1;

        uint8_t colour_index = 1; /* White */
        if (field == BUILDERS || field == SOLDIERS || field == MAGI)
        {
            colour_index = 2; /* Yellow */
        }

        /* Start by populating the pattern buffer with the panel background tiles */
        memcpy (buffer_l,  &panel_patterns [panel_indices [field_backgrounds [field] + 0] << 3], 32);
        memcpy (buffer_r, &panel_patterns [panel_indices [field_backgrounds [field] + 1] << 3], 32);

        /* Draw the digit font into the pattern */
        for (uint8_t line = 0; line < 5; line++)
        {
            uint8_t font_line_l = 0;
            uint8_t font_line_r = 0;

            /* One digit */
            if (value <= 9)
            {
                font_line_l = digit_font [digit_0] [line];
            }
            /* Two digits */
            else if (value <= 99)
            {
                font_line_l = (digit_font [digit_1] [line] << 2) | (digit_font [digit_0] [line] >> 2);
                font_line_r = (digit_font [digit_0] [line] << 6);

            }
            /* Three digits */
            else
            {
                font_line_l = (digit_font [digit_2] [line] << 4) | (digit_font [digit_1] [line]);
                font_line_r = digit_font [digit_0] [line]  << 4;

            }

            for (uint8_t bitplane = 0; bitplane < 4; bitplane++)
            {
                buffer_l [(line + y_offset) * 4 + bitplane] &= ~font_line_l;
                buffer_r [(line + y_offset) * 4 + bitplane] &= ~font_line_r;

                if (colour_index & (1 << bitplane))
                {
                    buffer_l [(line + y_offset) * 4 + bitplane] |=  font_line_l;
                    buffer_r [(line + y_offset) * 4 + bitplane] |=  font_line_r;
                }
            }
        }

        /* TODO: Consider waiting for v-sync before writing to VRAM. This would need to
         *       be done outside of the loop to allow all values to change simultaneously. */
        SMS_loadTiles (buffer_l, PATTERN_PANEL_DIGITS + (player << 4) + (field << 1),     sizeof (buffer_l));
        SMS_loadTiles (buffer_r, PATTERN_PANEL_DIGITS + (player << 4) + (field << 1) + 1, sizeof (buffer_r));
    }

    memcpy (cache [0], resources [0], sizeof (cache [0]));
    memcpy (cache [1], resources [1], sizeof (cache [1]));
}


