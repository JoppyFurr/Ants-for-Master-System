/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 *
 * Title screen implementation
 */

#include <stdbool.h>
#include <stdint.h>
#include "SMSlib.h"

#define TARGET_SMS
#include "bank_2.h"
#include "../title_tile_data/palette.h"
#include "../title_tile_data/pattern_index.h"

#include "vram.h"
#include "title.h"

#define PORT_A_DPAD_MASK    (PORT_A_KEY_UP | PORT_A_KEY_DOWN | PORT_A_KEY_LEFT | PORT_A_KEY_RIGHT)
#define PORT_A_KEY_MASK     (PORT_A_KEY_1 | PORT_A_KEY_2)

typedef enum cursor_position_e {
    CURSOR_POS_BLACKS = 0,
    CURSOR_POS_REDS,
    CURSOR_POS_2_PLAYER,
    CURSOR_POS_DEMO,
    CURSOR_POS_INFINITE,
    CURSOR_POS_START,
    CURSOR_POS_MAX
} cursor_position_t;


/*
 * Draw the title screen cursor.
 */
static void draw_cursor (uint8_t x, uint8_t y)
{

    SMS_initSprites ();

    /* Cursor patterns are loaded at the base of vram */
    SMS_addSprite (x,     y,     cursor_panels [0] [0]);
    SMS_addSprite (x + 8, y,     cursor_panels [0] [1]);
    SMS_addSprite (x,     y + 8, cursor_panels [0] [2]);
    SMS_addSprite (x + 8, y + 8, cursor_panels [0] [3]);

    SMS_copySpritestoSAT ();
}


/*
 * Run the title screen / menu.
 */
void title_screen (void)
{
    uint16_t vdp_title_indices [768];
    cursor_position_t cursor_pos = CURSOR_POS_START;


    SMS_loadBGPalette (background_palette);
    SMS_loadSpritePalette (sprite_palette);
    SMS_setBackdropColor (0);
    SMS_mapROMBank (2); /* Title patterns are stored in bank 2 */

    SMS_loadTiles (cursor_patterns, PATTERN_CURSOR, sizeof (cursor_patterns));
    SMS_loadTiles (title_patterns, PATTERN_TITLE_IMAGE, sizeof (title_patterns));

    for (uint16_t i = 0; i < 768; i++)
    {
        vdp_title_indices [i] = title_indices [i] + PATTERN_TITLE_IMAGE;
    }
    SMS_loadTileMapArea (0, 0, vdp_title_indices, 32, 24);

    SMS_useFirstHalfTilesforSprites (true);
    draw_cursor (235, 175);

    SMS_displayOn ();

    /* For now, just wait for the player to press a button */
    while (true)
    {
        uint16_t key_pressed = SMS_getKeysPressed ();

        if (key_pressed & PORT_A_DPAD_MASK)
        {
            if ((key_pressed & PORT_A_KEY_UP) && cursor_pos > CURSOR_POS_BLACKS)
            {
                cursor_pos -= 1;
            }
            else if ((key_pressed & PORT_A_KEY_DOWN) && cursor_pos < CURSOR_POS_START)
            {
                cursor_pos += 1;
            }
            switch (cursor_pos)
            {
                case CURSOR_POS_BLACKS:
                    draw_cursor (208, 36);
                    break;
                case CURSOR_POS_REDS:
                    draw_cursor (208, 52);
                    break;
                case CURSOR_POS_2_PLAYER:
                    draw_cursor (208, 68);
                    break;
                case CURSOR_POS_DEMO:
                    draw_cursor (208, 84);
                    break;
                case CURSOR_POS_INFINITE:
                    draw_cursor (208, 140);
                    break;
                case CURSOR_POS_START:
                    draw_cursor (235, 175);
                    break;
                default:
                    break;
            }
        }

        if (key_pressed & PORT_A_KEY_MASK)
        {
            break;
        }

        SMS_waitForVBlank ();
    }
}
