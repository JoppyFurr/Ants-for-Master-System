/*
 * Sneptile
 * Joppy Furr 2024
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sneptile.h"

/* State */
static uint32_t pattern_index = 0;
static uint32_t line_pattern_index = 0;
static uint32_t line_ct_index = 0;
static const char *input_filename = NULL;
static bool first_pattern_in_file = false;

/* Current colour-table entry to emit. */
static uint8_t ct_entry [2] = { };
static uint32_t ct_entry_size = 0;

/* Secondary colour-table entry, used to check if a
 * tile is compatible with the limitations of mode-0. */
static uint8_t test_ct_entry [2] = { };
static uint32_t test_ct_entry_size = 0;

/* Mode-4 Output Files */
static FILE *pattern_file = NULL;
static FILE *pattern_index_file = NULL;
static FILE *colour_table_file = NULL;

/* TMS9928a palette (gamma corrected) */
static const pixel_t tms9928a_palette [16] = {
    { .r = 0x00, .g = 0x00, .b = 0x00 },    /* Transparent */
    { .r = 0x00, .g = 0x00, .b = 0x00 },    /* Black */
    { .r = 0x0a, .g = 0xad, .b = 0x1e },    /* Medium Green */
    { .r = 0x34, .g = 0xc8, .b = 0x4c },    /* Light Green */
    { .r = 0x2b, .g = 0x2d, .b = 0xe3 },    /* Dark Blue */
    { .r = 0x51, .g = 0x4b, .b = 0xfb },    /* Light Blue */
    { .r = 0xbd, .g = 0x29, .b = 0x25 },    /* Dark Red */
    { .r = 0x1e, .g = 0xe2, .b = 0xef },    /* Cyan */
    { .r = 0xfb, .g = 0x2c, .b = 0x2b },    /* Medium Red */
    { .r = 0xff, .g = 0x5f, .b = 0x4c },    /* Light Red */
    { .r = 0xbd, .g = 0xa2, .b = 0x2b },    /* Dark Yellow */
    { .r = 0xd7, .g = 0xb4, .b = 0x54 },    /* Light Yellow */
    { .r = 0x0a, .g = 0x8c, .b = 0x18 },    /* Dark Green */
    { .r = 0xaf, .g = 0x32, .b = 0x9a },    /* Magenta */
    { .r = 0xb2, .g = 0xb2, .b = 0xb2 },    /* Grey */
    { .r = 0xff, .g = 0xff, .b = 0xff }     /* White */
};


/*
 * Open the three output files.
 */
int tms9928a_open_files (void)
{
    char *patterns_path = "patterns.h";
    char *pattern_index_path = "pattern_index.h";
    char *colour_table_path = "colour_table.h";

    /* Give sprite modes a different name, so that
     * they can be used in the same project alongside
     * background tiles. */
    if (target == VDP_MODE_TMS_SMALL_SPRITES)
    {
        patterns_path = "sprites.h";
        pattern_index_path = "sprite_index.h";
    }
    else if (target == VDP_MODE_TMS_LARGE_SPRITES)
    {
        patterns_path = "sprites_l.h";
        pattern_index_path = "sprite_index_l.h";
    }

    /* If the user has specified an output
     * directory, create and change into it */
    if (output_dir != NULL)
    {
        asprintf (&patterns_path, "%s/%s", output_dir, patterns_path);
        asprintf (&pattern_index_path, "%s/%s", output_dir, pattern_index_path);
        asprintf (&colour_table_path, "%s/%s", output_dir, colour_table_path);
    }

    /* Pattern file */
    pattern_file = fopen (patterns_path, "w");
    if (pattern_file == NULL)
    {
        fprintf (stderr, "Unable to open output file patterns.h\n");
        return RC_ERROR;
    }

    if (target == VDP_MODE_TMS_SMALL_SPRITES)
    {
        fprintf (pattern_file, "static const uint32_t sprites [] = {\n");
    }
    else if (target == VDP_MODE_TMS_LARGE_SPRITES)
    {
        fprintf (pattern_file, "static const uint32_t sprites_l [] = {\n");
    }
    else
    {
        fprintf (pattern_file, "static const uint32_t patterns [] = {\n");
    }

    /* Pattern index file */
    pattern_index_file = fopen (pattern_index_path, "w");
    if (pattern_index_file == NULL)
    {
        fprintf (stderr, "Unable to open output file pattern_index.h\n");
        return RC_ERROR;
    }

    /* Mode-0 and Mode-2 tile maps use a colour table, but sprites do not. */
    if (target == VDP_MODE_0 || target == VDP_MODE_2)
    {
        /* Colour table file */
        colour_table_file = fopen (colour_table_path, "w");
        if (colour_table_file == NULL)
        {
            fprintf (stderr, "Unable to open output file colour_table.h\n");
            return RC_ERROR;
        }
        fprintf (colour_table_file, "static const %s colour_table [] = {\n", (target == VDP_MODE_0) ? "uint8_t" : "uint32_t");
    }

    if (output_dir != NULL)
    {
        free (patterns_path);
        free (pattern_index_path);
        free (colour_table_path);
    }

    return RC_OK;
}


/*
 * Output one pattern to the pattern file.
 */
static void tms9928a_emit_pattern (uint8_t *pattern_lines)
{
    /* Indent at the start of each line, plus spaces between 4-byte words. */
    fprintf (pattern_file, "%s", line_pattern_index == 0 ? "    " : " ");

    fprintf (pattern_file, "0x%02x%02x%02x%02x, 0x%02x%02x%02x%02x,",
             pattern_lines [3], pattern_lines [2], pattern_lines [1], pattern_lines [0],
             pattern_lines [7], pattern_lines [6], pattern_lines [5], pattern_lines [4]);

    line_pattern_index++;

    if (line_pattern_index == 4)
    {
        fprintf (pattern_file, "\n");
        line_pattern_index = 0;
    }
}


/*
 * Output an entry to the mode-0 colour table file.
 */
static void tms9928a_mode0_emit_ct_entry (void)
{
    /* Eight entries per line. Indent at the start of each line, plus spaces between entries. */
    fprintf (colour_table_file, "%s", line_ct_index == 0 ? "    " : " ");

    fprintf (colour_table_file, "0x%02x,", (ct_entry [0] & 0x0f) | ((ct_entry [1] << 4) & 0xf0));
    line_ct_index++;

    if (line_ct_index == 8)
    {
        fprintf (colour_table_file, "\n");
        line_ct_index = 0;
    }
}


/*
 * Output an entry to the mode-2 colour table file.
 */
static void tms9928a_mode2_emit_ct_entry (uint8_t *ct_lines)
{
    /* Indent at the start of each line, plus spaces between 4-byte words. */
    fprintf (colour_table_file, "%s", line_ct_index == 0 ? "    " : " ");

    fprintf (colour_table_file, "0x%02x%02x%02x%02x, 0x%02x%02x%02x%02x,",
             ct_lines [3], ct_lines [2], ct_lines [1], ct_lines [0],
             ct_lines [7], ct_lines [6], ct_lines [5], ct_lines [4]);

    line_ct_index++;

    if (line_ct_index == 4)
    {
        fprintf (colour_table_file, "\n");
        line_ct_index = 0;
    }
}


/*
 * Finalize and Close the three output files.
 */
int tms9928a_close_files (void)
{
    /* Pattern file */
    fprintf (pattern_file, "%s};\n", line_pattern_index != 0 ? "\n" : "");
    fclose (pattern_file);
    pattern_file = NULL;

    /* Pattern index file */
    fclose (pattern_index_file);
    pattern_index_file = NULL;

    if (colour_table_file != NULL)
    {
        /* Colour table file */
        if (target == VDP_MODE_0 && pattern_index % 8 != 0)
        {
            tms9928a_mode0_emit_ct_entry ();
        }

        fprintf (colour_table_file, "%s};\n", line_ct_index != 0 ? "\n" : "");
        fclose (colour_table_file);
        colour_table_file = NULL;
    }

    return RC_OK;
}


/*
 * Mark the start of a new source file.
 */
void tms9928a_new_input_first_tile (void)
{
    const char *name = input_filename;

    /* Mark in patterns file */
    fprintf (pattern_file, "%s\n    /* %s */\n", line_pattern_index != 0 ? "\n" : "", name);
    line_pattern_index = 0;

    if (target == VDP_MODE_2)
    {
        /* Mark in colour-table file */
        fprintf (colour_table_file, "%s\n    /* %s */\n", line_ct_index != 0 ? "\n" : "", name);
        line_ct_index = 0;
    }

    /* Generate pattern index define */
    fprintf (pattern_index_file, "#define PATTERN_");

    for (char c = *name; *name != '\0'; c = *++name)
    {
        /* Don't include the file extension */
        if (c == '.')
        {
            break;
        }
        if (!isalnum (c))
        {
            c = '_';
        }

        fprintf (pattern_index_file, "%c", toupper(c));
    }

    fprintf (pattern_index_file, " %d\n", pattern_index);

    first_pattern_in_file = false;
}


/*
 * Mark the start of a new source file.
 *
 * Note that the actual marking occurs later, as we may need
 * to generate padding tiles before the first tile from this file.
 */
void tms9928a_new_input_file (const char *name)
{
    input_filename = name;
    first_pattern_in_file = true;
}


/*
 * Convert from pixel colour to the indexed tms9928a colour.
 * Assumes that the input is using the gamma-corrected values.
 */
static uint8_t tms9928a_rgb_to_colour_index (pixel_t p)
{
    static bool warn_once = true;

    if (p.a != 0)
    {
        /* Map from RGB to tms9928a colour */
        for (int tms_colour = 1; tms_colour < 16; tms_colour++)
        {
            if (p.r == tms9928a_palette [tms_colour].r &&
                p.g == tms9928a_palette [tms_colour].g &&
                p.b == tms9928a_palette [tms_colour].b)
            {
                return tms_colour;
            }
        }

        /* Warn if a non-compatible colour is used. */
        if (warn_once)
        {
            fprintf (stderr, "Warning: Image contains invalid colours for tms9928a.\n");
            warn_once = false;
        }
    }

    /* Zero for transparency */
    return 0;
}


/*
 * Convert from pixel colour to tms9928a pattern bit.
 * Returns 0 for background colour.
 * Returns 1 for foreground colour.
 */
static uint8_t tms9928a_rgb_to_ct_bit (pixel_t p)
{
    uint8_t colour = tms9928a_rgb_to_colour_index (p);

    /* For sprites, all we care about is whether the pixel is transparent or not */
    if (target == VDP_MODE_TMS_SMALL_SPRITES || target == VDP_MODE_TMS_LARGE_SPRITES)
    {
        return colour != 0;
    }

    /* Check if the colour is already in the colour-table byte */
    for (uint32_t i = 0; i < ct_entry_size; i++)
    {
        if (ct_entry [i] == colour)
        {
            return i;
        }
    }

    /* If not, add it. We avoid adding a third colour by checking
     * compatibility at the start of the tile's processing. */
    ct_entry [ct_entry_size] = colour;
    return ct_entry_size++;
}


/*
 * Generate a one-tile colour-table entry, used for checking
 * compatibility within a mode-0 block of eight.
 */
static void tms9928a_generate_ct_test_entry (pixel_t *buffer, uint32_t stride, uint32_t lines)
{
    test_ct_entry_size = 0;

    for (uint32_t y = 0; y < lines; y++)
    {
        for (uint32_t x = 0; x < 8; x++)
        {
            uint8_t colour = 0;
            pixel_t p = buffer [x + y * stride];

            colour = tms9928a_rgb_to_colour_index (p);

            /* Check if the colour is already in the colour-table byte */
            if ((test_ct_entry_size >= 1 && colour == test_ct_entry [0]) ||
                (test_ct_entry_size >= 2 && colour == test_ct_entry [1]))
            {
                continue;
            }

            /* If not, add it */
            if (test_ct_entry_size < 2)
            {
                test_ct_entry [test_ct_entry_size++] = colour;
            }
            else
            {
                /* Mark the size as too big and return. */
                test_ct_entry_size++;
                return;
            }
        }
    }
}


/*
 * Compare the current colour-table entry against the
 * test-entry to see if they are compatible.
 *
 * Expects that when called, each ct_entry contains no more than two colours.
 */
static bool tms9928a_check_ct_compatible (void)
{
    /* A fresh palette is always safe */
    if (ct_entry_size == 0)
    {
        return true;
    }

    /* If the palette already contains one colour, then
     * the new tile may add up to one additional colour. */
    else if (ct_entry_size == 1)
    {
        return (test_ct_entry_size == 1 ||
                test_ct_entry [0] == ct_entry [0] || test_ct_entry [1] == ct_entry [0]);
    }

    /* If the palette already contains two colours, then
     * the new tile may not add any additional colours. */
    else if (ct_entry_size == 2)
    {
        return (test_ct_entry_size == 1 && (test_ct_entry [0] == ct_entry [0] || test_ct_entry [0] == ct_entry [1])) ||
               (test_ct_entry [0] == ct_entry [0] && test_ct_entry [1] == ct_entry [1]) ||
               (test_ct_entry [0] == ct_entry [1] && test_ct_entry [1] == ct_entry [0]);

    }

    /* Should be unreachable. */
    return false;
}


/*
 * Process a single 8×8 tile.
 */
static void tms9928a_process_tile_8 (pixel_t *buffer, uint32_t stride)
{
    uint8_t pattern_lines [8] = { };
    uint8_t pattern_colours [8] = { }; /* For mode-2 */

    if (target == VDP_MODE_0)
    {
        /* First, generate the palette we'd need for this tile so that
         * we can check it against the limitations of the mode-0. */
        tms9928a_generate_ct_test_entry (buffer, stride, 8);

        /* In mode-0, each tile is allowed only two colours. */
        if (test_ct_entry_size > 2)
        {
            fprintf (stderr, "Error: Tile contains too many colours for mode-0.\n");
            return;
        }

        /* If the colours are not compatible, we need to emit dummy
         * tiles until we reach the next block of eight patterns so
         * that the palette can be reset. */
        if (!tms9928a_check_ct_compatible ())
        {
            if (pattern_index % 8 != 0)
            {
                while (pattern_index % 8 != 0)
                {
                    tms9928a_emit_pattern (pattern_lines);
                    pattern_index++;
                }
                tms9928a_mode0_emit_ct_entry ();
            }
            ct_entry_size = 0;
        }
    }

    for (uint32_t y = 0; y < 8; y++)
    {
        /* Each pattern line on mode-2 gets its own colour table entry */
        if (target == VDP_MODE_2)
        {
            /* Check if this line contains more than two colours. */
            tms9928a_generate_ct_test_entry (&buffer [y * stride], stride, 1);
            if (test_ct_entry_size > 2)
            {
                fprintf (stderr, "Error: Line contains too many colours for mode-0.\n");
                return;
            }
            ct_entry_size = 0;
        }

        for (uint32_t x = 0; x < 8; x++)
        {
            pixel_t p = buffer [x + y * stride];
            uint8_t bit = tms9928a_rgb_to_ct_bit (p);

            /* Convert to 1-bit-per-pixel representation */
            if (bit)
            {
                pattern_lines [y] |= (1 << (7 - x));
            }
        }

        /* Each pattern line on mode-2 gets its own colour table entry */
        if (target == VDP_MODE_2)
        {
            pattern_colours [y] = (ct_entry [0] & 0x0f) | ((ct_entry [1] << 4) & 0xf0);
        }
    }

    /* If this is the first pattern generated for our input file,
     * mark it in the pattern file and generate the index definition. */
    if (first_pattern_in_file)
    {
        tms9928a_new_input_first_tile ();
    }
    tms9928a_emit_pattern (pattern_lines);

    /* Emit a colour-table entry */
    if (target == VDP_MODE_0)
    {
        if (pattern_index % 8 == 7)
        {
            tms9928a_mode0_emit_ct_entry ();
        }
    }
    else if (target == VDP_MODE_2)
    {
        tms9928a_mode2_emit_ct_entry (pattern_colours);
    }

    pattern_index++;
}


/*
 * Process a single tile.
 * The tile size is 8×8 for the tile-map and small sprites.
 * The tile size is 16×16 for large sprites.
 */
void tms9928a_process_tile (pixel_t *buffer)
{
    uint32_t stride = current_image.width;

    if (target == VDP_MODE_TMS_LARGE_SPRITES)
    {
        /* Sprite layout: 0 2
         *                1 3 */
        tms9928a_process_tile_8 (&buffer [0             ], stride);
        tms9928a_process_tile_8 (&buffer [0 + 8 * stride], stride);
        tms9928a_process_tile_8 (&buffer [8             ], stride);
        tms9928a_process_tile_8 (&buffer [8 + 8 * stride], stride);
    }
    else
    {
        tms9928a_process_tile_8 (buffer, stride);
    }
}
