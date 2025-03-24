/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "SMSlib.h"
#include "../libraries/sms-fxsample/fxsample.h"

#define TARGET_SMS
#include "bank_4.h"
#include "../game_tile_data/pattern_index.h"
#include "../card_tile_data/pattern_index.h"

#include "vram.h"
#include "cards.h"
#include "game.h"
#include "save.h"
#include "rng.h"
#include "sound.h"
#include "title.h"

/* Shared state */
bool reset = false;

/* External variables */
extern uint16_t resources [2] [FIELD_MAX];

/* Empty card area */
static const uint16_t empty_slot [24] = {
    PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK,
    PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK,
    PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK,
    PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK,
    PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK,
    PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK
};

/* Ten cards are loaded into VRAM at a time:
 * 0-7: Player's hand
 *   8: Most recent discard
 *   9: Card-back */
static uint16_t card_buffer [10] [24];
uint8_t card_buffer_contains [10] = {
    CARD_NONE, CARD_NONE, CARD_NONE, CARD_NONE,
    CARD_NONE, CARD_NONE, CARD_NONE, CARD_NONE,
    CARD_NONE, CARD_NONE
};

/* A reference to which card_buffer slot is being used as a sprite */
static uint8_t card_buffer_sprite = 0;


/*
 * For now, the ten buffered cards are stored without de-duplication,
 * using a fixed 24-pattern block per card.
 */
void card_buffer_prepare (void)
{
    /* Dynamic buffer in VRAM for patterns */
    uint16_t index = 0;
    for (uint8_t slot = 0; slot < 10; slot++)
    {
        for (uint8_t pattern = 0; pattern < 24; pattern++)
        {
            card_buffer [slot] [pattern] = 0x0800 | PATTERN_CARD_BUFFER + index++;
        }

        /* Reset the software reference */
        card_buffer_contains [slot] = CARD_NONE;
    }
}


/* Slide animation variables */
uint16_t slide_start_x = 0;
uint16_t slide_start_y = 0;
card_t slide_card = 0;


/*
 * Load a card's patterns into the specified buffer slot in vram.
 * The return value is the actual buffer used.
 */
static inline uint8_t load_card (card_t card, uint8_t slot)
{
    /* Always use the final slot for the card back */
    if (card == CARD_BACK)
    {
        slot = 9;
    }

    /* Only update the data in VRAM if the card in this buffer-slot has changed */
    if (card_buffer_contains [slot] != card)
    {
        card_buffer_contains [slot] = card;
        SMS_mapROMBank (4); /* Card artwork is located in bank 4. */

        if (card & DISCARD_BIT)
        {
            card = card & CARD_BITS;

            const uint16_t *discard_panels = (card < 10) ? &cards_panels [CARD_DISCARD] [0] :
                                             (card < 20) ? &cards_panels [CARD_DISCARD] [8] :
                                                           &cards_panels [CARD_DISCARD] [16];

            /* Top section (unchanged) */
            for (uint8_t i = 0; i < 16; i++)
            {
                uint16_t pattern = cards_panels [card] [i] << 3;
                SMS_loadTiles (&cards_patterns [pattern], card_buffer [slot][i] & 0x01ff, 32);
            }

            /* Middle section (half-and-half) */
            for (uint8_t i = 0; i < 4; i++)
            {
                uint32_t pattern_buffer [8];
                uint16_t card_pattern = cards_panels [card] [i + 16] << 3;
                uint16_t discard_pattern = discard_panels [i] << 3;

                /* Three lines from the card pattern, and five lines from the discard pattern */
                memcpy (&pattern_buffer [0], &cards_patterns [card_pattern    + 0], 3 << 2);
                memcpy (&pattern_buffer [3], &cards_patterns [discard_pattern + 3], 5 << 2);
                SMS_loadTiles (pattern_buffer, card_buffer [slot][i + 16] & 0x01ff, 32);
            }

            /* Bottom section (discard-text) */
            for (uint8_t i = 4; i < 8; i++)
            {
                uint16_t pattern = discard_panels [i] << 3;
                SMS_loadTiles (&cards_patterns [pattern], card_buffer [slot][i + 16] & 0x01ff, 32);
            }
        }
        else
        {
            /* copy pattern bytes to VRAM */
            for (uint8_t i = 0; i < 24; i++)
            {
                uint16_t pattern = cards_panels [card] [i] << 3;
                SMS_loadTiles (&cards_patterns [pattern], card_buffer [slot][i] & 0x01ff, 32);
            }
        }
        SMS_mapROMBank (3); /* By default, keep the gameplay VDP patterns mapped */
    }

    return slot;
}


static inline void render_card_as_sprite_prepare (card_t card, uint8_t slot)
{
    card_buffer_sprite = load_card (card, slot);
}


/*
 * Draw the card, already in the reserved pattern area, using sprites.
 * TODO: Consider using SMS_updateSpritePosition.
 */
static inline void render_card_as_sprite (uint16_t x, uint16_t y)
{
    uint8_t pattern_index = 0;

    for (uint8_t panel_y = 0; panel_y < 6; panel_y++)
    for (uint8_t panel_x = 0; panel_x < 4; panel_x++)
    {
        SMS_addSprite (x + (panel_x << 3),
                       y + (panel_y << 3), card_buffer [card_buffer_sprite] [pattern_index++]);
    }
}


/*
 * Render a card to the background.
 * TODO: Temporarily taking in the slot number to simplify caching.
 */
void render_card_as_background (uint8_t x, uint8_t y, card_t card, uint8_t slot)
{
    if (card == CARD_NONE)
    {
        SMS_loadTileMapArea (x, y, empty_slot, 4, 6);
    }
    else
    {
        slot = load_card (card, slot);
        SMS_loadTileMapArea (x, y, card_buffer [slot], 4, 6);
    }
}


/*
 * Prepare for the card sliding animation.
 * This will render the card as a sprite in the starting position.
 */
void card_slide_from (uint16_t start_x, uint16_t start_y, card_t card, uint8_t slot)
{
    slide_start_x = start_x;
    slide_start_y = start_y;

    SMS_initSprites ();
    render_card_as_sprite_prepare (card, slot);
    render_card_as_sprite (start_x, start_y);

    SMS_waitForVBlank ();
    SMS_copySpritestoSAT ();
}


/*
 * Animate a card sliding from the start position to the end
 * position, leaving it rendered as a sprite.
 */
void card_slide_to (uint16_t end_x, uint16_t end_y)
{
    uint16_t x;
    uint16_t y;

    x = slide_start_x << 5;
    y = slide_start_y << 5;

    /* Play the card sound with each animation. */
    play_card_sound ();

    for (uint8_t frame = 0; frame < 32; frame++)
    {
        x += end_x - slide_start_x;
        y += end_y - slide_start_y;

        SMS_initSprites ();
        render_card_as_sprite (x >> 5, y >> 5);

        /* Write the new sprite position only during vblank. */
        SMS_waitForVBlank ();
        SMS_copySpritestoSAT ();
    }

    /* Note, we don't clear the sprite here. First the
     * caller needs to draw the card into the background. */
}


/*
 * Animate a card sliding from the start position to the end
 * position, leaving it rendered as a sprite.
 */
void card_slide_to_fast (uint16_t end_x, uint16_t end_y)
{
    uint16_t x;
    uint16_t y;

    x = slide_start_x << 4;
    y = slide_start_y << 4;

    /* Play the card sound with each animation. */
    play_card_sound ();

    for (uint8_t frame = 0; frame < 16; frame++)
    {
        x += end_x - slide_start_x;
        y += end_y - slide_start_y;

        SMS_initSprites ();
        render_card_as_sprite (x >> 4, y >> 4);

        /* Write the new sprite position only during vblank. */
        SMS_waitForVBlank ();
        SMS_copySpritestoSAT ();
    }

    /* Note, we don't clear the sprite here. First the
     * caller needs to draw the card into the background. */
}


/*
 * Clear the card sprite animation. This should be called
 * once the card has been drawn to the background.
 */
void card_slide_done (void)
{
    SMS_initSprites ();
    SMS_copySpritestoSAT ();
}


/*
 * Delay until the chosen number of frames have passed.
 */
void delay_frames (uint8_t frames)
{
    while (frames)
    {
        SMS_waitForVBlank ();
        frames--;
    }
}


/*
 * Frame interrupt.
 * currently used to check if the reset button has been pressed.
 */
static void frame_interrupt (void)
{
    if (SMS_getKeysPressed () & RESET_KEY)
    {
        reset = true;
    }
}


/*
 * Entry point.
 */
void main (void)
{
    const uint8_t psg_init [] = {
        0x9f, 0xbf, 0xdf, 0xff, 0x81, 0x00, 0xa1, 0x00, 0xc1, 0x00, 0xe0
    };

    /* Once-off setup */
    SMS_setBackdropColor (0);
    initPSG ((void *) psg_init);
    sram_load ();
    SMS_useFirstHalfTilesforSprites (true);
    SMS_setFrameInterruptHandler (frame_interrupt);

    /* Outermost loop, cycles between the title screen and gameplay.
     * The RNG is seeded so that time spent at the title screen has
     * a chance to affect the R register.
     * The only exit from gameplay is the SMS-1 reset button. */
    while (true)
    {
        title_screen ();
        rng_seed ();
        reset = false;
        game_start ();
    }
}

SMS_EMBED_SEGA_ROM_HEADER(9999, 0);
