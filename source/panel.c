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
extern const uint16_t player_panels [] [8];
extern const uint32_t player_patterns [];
extern uint16_t player_patterns_start;

extern const uint16_t panel_indices [];
extern const uint32_t panel_patterns [];
extern uint16_t panel_patterns_start;

/* Game state */
extern uint16_t wins [2];
extern uint16_t resources [2] [FIELD_MAX];


/*
 * Update the win counters at the top of the screen.
 */
void panel_update_wins (uint8_t player)
{
    /* A pair of buffers to hold the patterns we will draw into,
     * initialised to black. */
    uint8_t buffer_l [32] = { 0 };
    uint8_t buffer_r [32] = { 0 };

    const uint16_t value = wins [player];
    if (value > 999)
    {
        /* Stop at three digits. */
        return;
    }

    const uint8_t digit_0 = value % 10;           /* Ones */
    const uint8_t digit_1 = (value % 100) / 10;   /* Tens */
    const uint8_t digit_2 = value / 100;          /* Hundreds */

    const uint8_t y_offset = 1;

    uint8_t colour_index = 4; /* Green */

    /* Draw the digit font into the pattern */
    for (uint8_t line = 0; line < 5; line++)
    {
        uint8_t font_line_l = 0;
        uint8_t font_line_r = 0;

        /* One digit */
        if (value <= 9)
        {
            font_line_l = digit_font [digit_0] [line] >> 2;
            font_line_r = digit_font [digit_0] [line] << 6;
        }
        /* Two digits */
        else if (value <= 99)
        {
            font_line_l = digit_font [digit_1] [line];
            font_line_r = digit_font [digit_0] [line] << 4;
        }
        /* Three digits */
        else
        {
            font_line_l = (digit_font [digit_2] [line] << 2) | (digit_font [digit_1] [line] >> 2);
            font_line_r = (digit_font [digit_1] [line] << 6) | (digit_font [digit_0] [line] << 2);
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

    /* TODO: Consider waiting for v-sync before writing to VRAM. */
    SMS_loadTiles (buffer_l, PATTERN_WIN_DIGITS + (player << 1),     sizeof (buffer_l));
    SMS_loadTiles (buffer_r, PATTERN_WIN_DIGITS + (player << 1) + 1, sizeof (buffer_r));
}


/*
 * Initialise the win counter.
 */
void panel_init_wins (void)
{
    const uint16_t blacks_win_digits [2] = {
        0x0800 | PATTERN_WIN_DIGITS + 0,
        0x0800 | PATTERN_WIN_DIGITS + 1,
    };
    SMS_loadTileMapArea (1, 0, blacks_win_digits, 2, 1);

    const uint16_t reds_win_digits [2] = {
        0x0800 | PATTERN_WIN_DIGITS + 2,
        0x0800 | PATTERN_WIN_DIGITS + 3
    };
    SMS_loadTileMapArea (29, 0, reds_win_digits, 2, 1);

    panel_update_wins (0);
    panel_update_wins (1);
}


/*
 * Update the player indicator at the top of the screen.
 */
void panel_update_player (uint8_t player)
{
    uint16_t left [8];
    uint16_t right [8];

    if (player == 0)
    {
        for (uint8_t i = 0; i < 8; i++)
        {
            left [i] = player_panels [2] [i] + player_patterns_start | 0x0800;
            right [i] = player_panels [1] [i] + player_patterns_start | 0x0800;
        }
    }
    else
    {
        for (uint8_t i = 0; i < 8; i++)
        {
            left [i] = player_panels [0] [i] + player_patterns_start | 0x0800;
            right [i] = player_panels [3] [i] + player_patterns_start | 0x0800;
        }
    }

    /* Write to VDP */
    SMS_loadTileMapArea (0, 1, left, 4, 2);
    SMS_loadTileMapArea (28, 1, right, 4, 2);
}



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
