/*
 * Sneptile
 * Joppy Furr 2024
 *
 * This is a tool to generate pattern data for the
 * Sega Master System VDP, from a set of .png images.
 *
 * To Do list:
 *  - 'sprite mode' to not match on palette index 0
 *  - 'tall sprite mode' vertical tile ordering
 *  - Option for removing duplicate tiles
 *  - Option to generate tile maps for larger images
 *  - Option to help automate colour-cycling
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
char *output_dir = NULL;


/*
 * Process an image made up of 8×8 tiles.
 */
static int sneptile_process_image (pixel_t *buffer, uint32_t image_width, uint32_t image_height, char *name)
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
            mode4_new_input_file (name);
            break;
        default:
            break;
    }

    /* Sanity check */
    if ((image_width % tile_width != 0) || (image_height % tile_height != 0))
    {
        fprintf (stderr, "Error: Invalid resolution %ux%u\n", image_width, image_height);
        return -1;
    }

    for (uint32_t row = 0; row < image_height; row += tile_height)
    {
        for (uint32_t col = 0; col < image_width; col += tile_width)
        {
            switch (target)
            {
                case VDP_MODE_0:
                case VDP_MODE_2:
                case VDP_MODE_TMS_SMALL_SPRITES:
                case VDP_MODE_TMS_LARGE_SPRITES:
                    tms9928a_process_tile (&buffer [row * image_width + col], image_width);
                    break;
                case VDP_MODE_4:
                    mode4_process_tile (&buffer [row * image_width + col], image_width);
                    break;
                default:
                    break;
            }
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
    if (sneptile_process_image ((pixel_t *) image_buffer, header.width, header.height, name) != 0)
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
        fprintf (stderr, "Usage: %s [--mode-0] [--output <dir>] [--palette <0x00 0x01..>] <tiles.png>\n", argv [0]);
        return EXIT_FAILURE;
    }
    argv++;
    argc--;

    /* VDP Mode */
    if (strcmp (argv [0], "--mode-0") == 0)
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

    /* User-specified output directory */
    if (strcmp (argv [0], "--output") == 0 && argc > 2)
    {
        output_dir = argv [1];
        argv += 2;
        argc -= 2;
    }

    /* User-initialized mode-4 palette */
    if (strcmp (argv [0], "--palette") == 0)
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
            rc = mode4_open_files ();
            break;
        default:
            break;
    }

    if (rc == RC_OK)
    {
        for (uint32_t i = 0; i < argc; i++)
        {
            rc = sneptile_process_file (argv [i]);
            if (rc != RC_OK)
            {
                break;
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
                rc = mode4_close_files ();
                break;
            default:
                break;
        }
    }

    return rc == RC_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
