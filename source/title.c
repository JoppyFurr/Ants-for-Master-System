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

/*
 * Run the title screen / menu.
 */
void title_screen (void)
{
    SMS_loadBGPalette (background_palette);
    SMS_loadSpritePalette (sprite_palette);
    SMS_setBackdropColor (0);
    SMS_mapROMBank (2); /* Title patterns are stored in bank 2 */

    SMS_loadTiles (title_patterns, PATTERN_TITLE_IMAGE, sizeof (title_patterns));
    SMS_loadTileMapArea (0, 0, title_indices, 32, 24);

    SMS_useFirstHalfTilesforSprites (true);
    SMS_initSprites ();
    SMS_copySpritestoSAT ();

    SMS_displayOn ();

    /* For now, just wait for the player to press a button */
    while (true)
    {
        uint16_t key_pressed = SMS_getKeysPressed ();

        if (key_pressed & PORT_A_KEY_MASK)
        {
            break;
        }

        SMS_waitForVBlank ();
    }
}
