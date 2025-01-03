/*
 * Sneptile
 * Joppy Furr 2024
 *
 * This is a tool to generate pattern data for the
 * Sega Master System VDP, from a set of .png images.
 *
 * To Do list:
 *  - 'tall sprite mode' vertical tile ordering
 *  - Option to help automate colour-cycling
 *  - Possible architecture change:
 *    -> Always de-duplicate
 *    -> De-duplicate into a tile-buffer
 *    -> Then, feed the de-duplicated tiles into the pattern generators.
 *    -> For mode-0, consider sorting the tiles by colours first as a pre-processing step
 *    -> Consider an external configuration file to describe panels within images.
 *       -> If present could also contain the file names instead of as parameters.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <spng.h>

#include "sneptile.h"
#include "sms_vdp.h"
#include "tms9928a.h"

/* Global State */
target_t target = VDP_MODE_4;
bool de_duplicate = false;
char *output_dir = NULL;
image_t current_image;

/* De-duplication */
void* unique_tiles [512];
uint32_t unique_tiles_count = 0;

/* Panes */
uint32_t panel_width = 0;
uint32_t panel_height = 0;
uint32_t panel_count = 0;


/*
 * Check if two 8x8 tiles are identical.
 * Note: Currently the two tiles must be within the same image file.
 */
static bool sneptile_check_match (pixel_t *tile_a, pixel_t *tile_b)
{
    for (uint32_t row = 0; row < 8; row++)
    {
        if (memcmp (&tile_a [row * current_image.width],
                    &tile_b [row * current_image.width], sizeof (pixel_t) * 8) != 0)
        {
            return false;
        }
    }

    return true;
}


/*
 * Find the matching 8x8 tile, or -1 if it is unique.
 */
int32_t sneptile_get_match (pixel_t *tile)
{
    for (uint32_t i = 0; i < unique_tiles_count; i++)
    {
        if (sneptile_check_match (tile, unique_tiles [i]))
            return i;
    }
    return -1;
}


/*
 * Process an image made up of 8×8 tiles.
 */
static int sneptile_process_image (pixel_t *buffer, char *name)
{
    uint32_t tile_width = 8;
    uint32_t tile_height = 8;

    switch (target)
    {
        case VDP_MODE_0:
        case VDP_MODE_2:
        case VDP_MODE_TMS_SMALL_SPRITES:
            tms9928a_new_input_file (name);
            break;
        case VDP_MODE_TMS_LARGE_SPRITES:
            tile_width = 16;
            tile_height = 16;
            tms9928a_new_input_file (name);
            break;
        case VDP_MODE_4:
        case VDP_MODE_4_SPRITES:
            mode4_new_input_file (name);
            break;
        default:
            break;
    }

    /* Sanity check */
    if ((current_image.width % tile_width != 0) || (current_image.height % tile_height != 0))
    {
        fprintf (stderr, "Error: Invalid resolution %ux%u\n", current_image.width, current_image.height);
        return -1;
    }

    /* Because we only store a pointer into the image buffer, we
     * currently can't de-duplicate across files. This should be
     * changed to store copies of the tiles. */
    unique_tiles_count = 0;

    for (uint32_t row = 0; row < current_image.height; row += tile_height)
    {
        for (uint32_t col = 0; col < current_image.width; col += tile_width)
        {

            if (de_duplicate && unique_tiles_count < 512)
            {
                if (sneptile_get_match (&buffer [row * current_image.width + col]) == -1)
                {
                    unique_tiles [unique_tiles_count++] = &buffer [row * current_image.width + col];
                }
                else
                {
                    continue;
                }
            }

            switch (target)
            {
                case VDP_MODE_0:
                case VDP_MODE_2:
                case VDP_MODE_TMS_SMALL_SPRITES:
                case VDP_MODE_TMS_LARGE_SPRITES:
                    tms9928a_process_tile (&buffer [row * current_image.width + col]);
                    break;
                case VDP_MODE_4:
                case VDP_MODE_4_SPRITES:
                    mode4_process_tile (&buffer [row * current_image.width + col]);
                    break;
                default:
                    break;
            }
        }
    }

    if (panel_count)
    {
        switch (target)
        {
            case VDP_MODE_4:
            case VDP_MODE_4_SPRITES:
                mode4_process_panels (name, panel_count, panel_width, panel_height, buffer);
            default:
                break;
        }
    }

    return 0;
}


/*
 * Process a single .png file.
 */
static int sneptile_process_file (char *name)
{
    spng_ctx *spng_context = spng_ctx_new (0);

    /* Try to open the file */
    FILE *png_file = fopen (name, "r");
    if (png_file == NULL)
    {
        fprintf (stderr, "Error: Unable to open %s.\n", name);
        return RC_ERROR;
    }

    /* Once the file has been opened, drop the path and use only the file name */
    if (strrchr (name, '/') != NULL)
    {
        name = strrchr (name, '/') + 1;
    }

    /* Get the file size */
    uint32_t png_size = 0;
    fseek (png_file, 0, SEEK_END);
    png_size = ftell (png_file);
    rewind (png_file);

    /* Allocate memory for the .png file */
    uint8_t *png_buffer = calloc (png_size, 1);
    if (png_buffer == NULL)
    {
        fprintf (stderr, "Error: Failed to allocate memory for %s.\n", name);
        return RC_ERROR;
    }

    /* Read and close the file */
    uint32_t bytes_read = 0;
    while (bytes_read < png_size)
    {
        bytes_read += fread (png_buffer + bytes_read, 1, png_size - bytes_read, png_file);
    }
    fclose (png_file);
    png_file = NULL;

    /* Get the decompressed image size */
    size_t image_size = 0;
    if (spng_set_png_buffer (spng_context, png_buffer, png_size) != 0)
    {
        fprintf (stderr, "Error: Failed to set file buffer for %s.\n", name);
        return RC_ERROR;
    }

    if (spng_decoded_image_size (spng_context, SPNG_FMT_RGBA8, &image_size) != 0)
    {
        fprintf (stderr, "Error: Failed to determine decompression size for %s.\n", name);
        return RC_ERROR;
    }

    /* Allocate memory for the decompressed image */
    uint8_t *image_buffer = calloc (image_size, 1);
    if (image_buffer == NULL)
    {
        fprintf (stderr, "Error: Failed to allocate decompression memory for %s.\n", name);
        return RC_ERROR;
    }

    /* Decode the image */
    if (spng_decode_image (spng_context, image_buffer, image_size, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS) != 0)
    {
        fprintf (stderr, "Error: Failed to decode image %s.\n", name);
        return RC_ERROR;
    }

    /* Process the image */
    struct spng_ihdr header = { };
    spng_get_ihdr(spng_context, &header);
    current_image.width = header.width;
    current_image.height = header.height;
    if (sneptile_process_image ((pixel_t *) image_buffer, name) != 0)
    {
        fprintf (stderr, "Error: Failed to process image %s.\n", name);
        return RC_ERROR;
    }

    /* Tidy up */
    free (png_buffer);
    free (image_buffer);
    spng_ctx_free (spng_context);

    return RC_OK;
}


/*
 * Entry point.
 */
int main (int argc, char **argv)
{
    int rc = 0;

    if (argc < 2)
    {
        fprintf (stderr, "Usage: %s [global options] [per-sheet options, tiles.png]\n", argv [0]);
        fprintf (stderr, "  Global options:\n");
        fprintf (stderr, "    --mode-0 : Generate TMS99xx mode-0 patterns\n");
        fprintf (stderr, "    --mode-2 : Generate TMS99xx mode-2 patterns\n");
        fprintf (stderr, "    --sprites : Mode-4 sprites. Index 0 will not be used for visible colours.\n");
        fprintf (stderr, "    --tms-small-sprites : Generate TMS99xx sprite patterns (8x8)\n");
        fprintf (stderr, "    --tms-large-sprites : Generate TMS99xx sprite patterns (16x16)\n");
        fprintf (stderr, "    --de-duplicate : Within an input file, don't generate the same pattern twice\n");
        fprintf (stderr, "    --output <dir> : Specify output directory\n");
        fprintf (stderr, "    --palette <0x00 0x01..> : Pre-defined palette entries.\n");
        fprintf (stderr, "  Per-sheet options:\n");
        fprintf (stderr, "    --reserve <name,n> : Reserve <n> patterns before the next sheet (eg, for runtime generated patterns)\n");
        fprintf (stderr, "    --panels <wxh,n> : The following sheet contains <n> panels of size <w> x <h>. Depends on de-duplication.\n");
        return EXIT_FAILURE;
    }
    argv++;
    argc--;


    while (argc > 0)
    {
        /* Common options */
        if (strcmp (argv [0], "--de-duplicate") == 0)
        {
            de_duplicate = true;
            argv += 1;
            argc -= 1;
        }
        else if (strcmp (argv [0], "--output") == 0 && argc > 2)
        {
            output_dir = argv [1];
            argv += 2;
            argc -= 2;
        }

        /* TMS99xx Options */
        else if (strcmp (argv [0], "--mode-0") == 0)
        {
            target = VDP_MODE_0;
            argv += 1;
            argc -= 1;
        }
        else if (strcmp (argv [0], "--mode-2") == 0)
        {
            target = VDP_MODE_2;
            argv += 1;
            argc -= 1;
        }
        else if (strcmp (argv [0], "--tms-small-sprites") == 0)
        {
            target = VDP_MODE_TMS_SMALL_SPRITES;
            argv += 1;
            argc -= 1;
        }
        else if (strcmp (argv [0], "--tms-large-sprites") == 0)
        {
            target = VDP_MODE_TMS_LARGE_SPRITES;
            argv += 1;
            argc -= 1;
        }

        /* SMS-GG Mode4 Options */
        else if (strcmp (argv [0], "--sprites") == 0)
        {
            target = VDP_MODE_4_SPRITES;
            argv += 1;
            argc -= 1;
        }
        else if (strcmp (argv [0], "--palette") == 0)
        {
            while (++argv, --argc)
            {
                if (strncmp (argv [0], "0x", 2) == 0 && strlen (argv[0]) == 4)
                {
                    mode4_palette_add_colour (strtol (argv [0], NULL, 16));
                }
                else
                {
                    break;
                }
            }
        }

        else
        {
            break;
        }
    }

    /* Create the output directory if one has been specified. */
    if (output_dir != NULL)
    {
        mkdir (output_dir, S_IRWXU);
    }

    /* Open the output files */
    switch (target)
    {
        case VDP_MODE_0:
        case VDP_MODE_2:
        case VDP_MODE_TMS_SMALL_SPRITES:
        case VDP_MODE_TMS_LARGE_SPRITES:
            rc = tms9928a_open_files ();
            break;
        case VDP_MODE_4:
        case VDP_MODE_4_SPRITES:
            rc = mode4_open_files ();
            break;
        default:
            break;
    }

    if (rc == RC_OK)
    {
        for (uint32_t i = 0; i < argc; i++)
        {
            if (strcmp (argv [i], "--reserve") == 0)
            {
                char name [80] = { '\0' };
                unsigned int count;
                sscanf (argv [++i], "%79[^,],%u", name, &count);

                /* For now, only implemented for mode-4 */
                switch (target)
                {
                    case VDP_MODE_4:
                    case VDP_MODE_4_SPRITES:
                        mode4_reserve_patterns (name, count);
                        break;
                    default:
                        break;
                }
            }
            else if (strcmp (argv [i], "--panels") == 0)
            {
                unsigned int width, height, count;
                sscanf (argv [++i], "%ux%u,%u", &width, &height, &count);
                panel_width = width;
                panel_height = height;
                panel_count = count;
            }
            else
            {
                rc = sneptile_process_file (argv [i]);
                if (rc != RC_OK)
                {
                    break;
                }

                /* Panel setting is only valid per-image */
                panel_count = 0;
            }
        }
    }

    if (rc == RC_OK)
    {
        /* Finalize and close the output files */
        switch (target)
        {
            case VDP_MODE_0:
            case VDP_MODE_2:
            case VDP_MODE_TMS_SMALL_SPRITES:
            case VDP_MODE_TMS_LARGE_SPRITES:
                rc = tms9928a_close_files ();
                break;
            case VDP_MODE_4:
            case VDP_MODE_4_SPRITES:
                rc = mode4_close_files ();
                break;
            default:
                break;
        }
    }

    return rc == RC_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
