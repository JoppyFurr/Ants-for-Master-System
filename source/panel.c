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
extern const uint16_t panel_panels [] [56];
extern const uint32_t panel_patterns [];
extern const uint32_t background_patterns [];

/* Game state */
extern uint16_t wins [2];
extern uint16_t resources [2] [FIELD_MAX];

/* Panel value cache */
static uint16_t panel_cache [2] [FIELD_MAX];

/*
 * Update the win counters at the top of the screen.
 */
void panel_update_wins (uint8_t player)
{
    /* A pair of buffers to hold the patterns we will draw into,
     * initialised to sky-blue from the first background pattern. */
    uint8_t buffer_l [32] = { 0 };
    uint8_t buffer_r [32] = { 0 };

    memcpy (buffer_l, background_patterns, 32);
    memcpy (buffer_r, background_patterns, 32);

    const uint16_t value = wins [player];
    if (value > 999)
    {
        /* Stop at three digits. */
        return;
    }

    const uint8_t digit_0 = value % 10;           /* Ones */
    const uint8_t digit_1 = (value % 100) / 10;   /* Tens */
    const uint8_t digit_2 = value / 100;          /* Hundreds */

    const uint8_t y_offset = 2;

    uint8_t colour_index = 12; /* Dark green */

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
        PATTERN_WIN_DIGITS + 0, PATTERN_WIN_DIGITS + 1,
    };
    SMS_loadTileMapArea (1, 0, blacks_win_digits, 2, 1);

    const uint16_t reds_win_digits [2] = {
        PATTERN_WIN_DIGITS + 2, PATTERN_WIN_DIGITS + 3
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
    SMS_mapROMBank (3);

    if (player == 0)
    {
        SMS_loadTiles (&player_patterns[0], PATTERN_PLAYERS, 512);
    }
    else
    {
        SMS_loadTiles (&player_patterns[128], PATTERN_PLAYERS, 512);
    }

}


/*
 * Sets up the name-table entries for the player indicators.
 */
void panel_init_player (void)
{
    uint16_t left [8];
    uint16_t right [8];

    for (uint8_t i = 0; i < 8; i++)
    {
        left [i] = player_panels [0] [i] + PATTERN_PLAYERS;
        right [i] = player_panels [1] [i] + PATTERN_PLAYERS;
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

    for (uint8_t player = 0; player < 2; player++)
    {
        for (uint8_t i = 0; i < 56; i++)
        {
            vdp_panel_indices [i] = panel_panels [player] [i] + PATTERN_PANELS;
        }

        /* The crystals need a shade of purple that's not in the background palette.
         * The palettes have been crafted so that for the crystal patterns can use
         * the sprite palette and sky-blue becomes purple. */
        vdp_panel_indices [29] |= 0x0800;
        vdp_panel_indices [33] |= 0x0800;

        uint8_t position = (player == 0) ? 0 : 28;
        SMS_loadTileMapArea (position, 3, vdp_panel_indices, 4, 14);
    }

    /* Set up panel digit areas */
    for (uint8_t i = 0; i < 16; i++)
    {
        /* Eight digit areas for each player */
        uint8_t x = (i < 8) ? 2 : 30;
        uint8_t y = y_positions [i & 0x07];

        uint16_t area [2] = { PATTERN_PANEL_DIGITS + (i << 1), PATTERN_PANEL_DIGITS + (i << 1) + 1 };
        SMS_loadTileMapArea (x, y, area, 2, 1);
    }

    /* Reset the cache */
    for (uint8_t resource = 0; resource < FIELD_MAX; resource++)
    {
        panel_cache [0] [resource] = 0;
        panel_cache [1] [resource] = 0;
    }
}


/*
 * Update the side-panel values.
 */
void panel_update (void)
{
    const uint8_t field_backgrounds [8] = { 6, 10, 18, 22, 30, 34, 46, 50 };

    /* A pair of buffers to hold the patterns we will draw into */
    uint8_t buffer_l [32];
    uint8_t buffer_r [32];

    for (uint8_t player = 0; player < 2; player++)
    for (uint8_t field = 0; field < FIELD_MAX; field++)
    {
        const uint16_t value = resources [player] [field];

        /* Skip fields that have not changed */
        if (value == panel_cache [player] [field])
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
        memcpy (buffer_l,  &panel_patterns [panel_panels [player] [field_backgrounds [field] + 0] << 3], 32);
        memcpy (buffer_r, &panel_patterns [panel_panels [player] [field_backgrounds [field] + 1] << 3], 32);

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

    memcpy (panel_cache [0], resources [0], sizeof (panel_cache [0]));
    memcpy (panel_cache [1], resources [1], sizeof (panel_cache [1]));
}
