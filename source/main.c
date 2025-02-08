/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "SMSlib.h"

#define TARGET_SMS
#include "../tile_data/palette.h"
#include "../tile_data/patterns.h"
#include "../tile_data/pattern_index.h"

#include "vram.h"
#include "cards.h"
#include "game.h"
#include "save.h"
#include "rng.h"

uint16_t player_patterns_start = 0;
uint16_t panel_patterns_start = 0;

/* External variables */
extern uint16_t resources [2] [FIELD_MAX];

/* Reserved patterns for card sprite use */
/* TODO: This is temporary. Right now, the card_buffer bellow doesn't do any de-duplication so
 *       needs more than 256 patterns if also including the sliding card. This prevents it from
 *       being used with sprites due to the 8-bit pattern index.
 *       Once de-duplication is used for a more flexible card_buffer, all of its in-VRAM patterns
 *       __should__ fit below 256, meaning we could remove card_sprite and instead use card_buffer. */
static const uint8_t card_sprite [24] = {
    PATTERN_CARD_SPRITE +  0, PATTERN_CARD_SPRITE +  1, PATTERN_CARD_SPRITE +  2, PATTERN_CARD_SPRITE +  3,
    PATTERN_CARD_SPRITE +  4, PATTERN_CARD_SPRITE +  5, PATTERN_CARD_SPRITE +  6, PATTERN_CARD_SPRITE +  7,
    PATTERN_CARD_SPRITE +  8, PATTERN_CARD_SPRITE +  9, PATTERN_CARD_SPRITE + 10, PATTERN_CARD_SPRITE + 11,
    PATTERN_CARD_SPRITE + 12, PATTERN_CARD_SPRITE + 13, PATTERN_CARD_SPRITE + 14, PATTERN_CARD_SPRITE + 15,
    PATTERN_CARD_SPRITE + 16, PATTERN_CARD_SPRITE + 17, PATTERN_CARD_SPRITE + 18, PATTERN_CARD_SPRITE + 19,
    PATTERN_CARD_SPRITE + 20, PATTERN_CARD_SPRITE + 21, PATTERN_CARD_SPRITE + 22, PATTERN_CARD_SPRITE + 23
};

/* A blank pattern */
static const uint32_t blank_pattern [8] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

/* Empty card area */
static const uint16_t empty_slot [24] = {
    PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK,
    PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK,
    PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK,
    PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK,
    PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK,
    PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK, PATTERN_BLANK
};

/* Reserved patterns for the card back. It's always in memory and uses 24 unique tiles. */
static const uint16_t card_back [24] = {
    0x0800 | PATTERN_CARD_BACK +  0, 0x0800 | PATTERN_CARD_BACK +  1, 0x0800 | PATTERN_CARD_BACK +  2, 0x0800 | PATTERN_CARD_BACK +  3,
    0x0800 | PATTERN_CARD_BACK +  4, 0x0800 | PATTERN_CARD_BACK +  5, 0x0800 | PATTERN_CARD_BACK +  6, 0x0800 | PATTERN_CARD_BACK +  7,
    0x0800 | PATTERN_CARD_BACK +  8, 0x0800 | PATTERN_CARD_BACK +  9, 0x0800 | PATTERN_CARD_BACK + 10, 0x0800 | PATTERN_CARD_BACK + 11,
    0x0800 | PATTERN_CARD_BACK + 12, 0x0800 | PATTERN_CARD_BACK + 13, 0x0800 | PATTERN_CARD_BACK + 14, 0x0800 | PATTERN_CARD_BACK + 15,
    0x0800 | PATTERN_CARD_BACK + 16, 0x0800 | PATTERN_CARD_BACK + 17, 0x0800 | PATTERN_CARD_BACK + 18, 0x0800 | PATTERN_CARD_BACK + 19,
    0x0800 | PATTERN_CARD_BACK + 20, 0x0800 | PATTERN_CARD_BACK + 21, 0x0800 | PATTERN_CARD_BACK + 22, 0x0800 | PATTERN_CARD_BACK + 23
};

/* The nine other cards currently loaded into VRAM.
 * Eight for the player's hand and one for the discard pile. */
static uint16_t card_buffer [9] [24];


/*
 * For now, the ten buffered cards are stored without de-duplication,
 * using a fixed 24-pattern block per card.
 */
static void card_buffer_prepare (void)
{
    /* Dynamic buffer */
    uint16_t index = 0;
    for (uint8_t slot = 0; slot < 9; slot++)
    {
        for (uint8_t pattern = 0; pattern < 24; pattern++)
        {
            card_buffer [slot] [pattern] = 0x0800 + PATTERN_CARD_BUFFER + index++;
        }
    }

    /* Fixed patterns for card-back buffer */
    for (uint8_t i = 0; i < 24; i++)
    {
        uint16_t pattern = cards_panels [30] [i] << 3;
        SMS_loadTiles (&cards_patterns [pattern], PATTERN_CARD_BACK + i, 32);
    }
}


/* Slide animation variables */
uint16_t slide_start_x = 0;
uint16_t slide_start_y = 0;
card_t slide_card = 0;


/*
 * Fill the name table with tile-zero.
 */
static inline void clear_background (void)
{
    uint16_t blank_line [32] = { 0 };

    for (uint8_t row = 0; row < 24; row++)
    {
        SMS_loadTileMapArea (0, row, &blank_line, 32, 1);
    }
}


/*
 * Draw the selected card into the reserved patterns.
 */
static inline void render_card_as_sprite_prepare (card_t card)
{
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
            SMS_loadTiles (&cards_patterns [pattern], card_sprite [i] & 0x01ff, 32);
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
            SMS_loadTiles (pattern_buffer, card_sprite [i + 16] & 0x01ff, 32);
        }

        /* Bottom section (discard-text) */
        for (uint8_t i = 4; i < 8; i++)
        {
            uint16_t pattern = discard_panels [i] << 3;
            SMS_loadTiles (&cards_patterns [pattern], card_sprite [i + 16] & 0x01ff, 32);
        }
    }
    else
    {
        for (uint8_t i = 0; i < 24; i++)
        {
            uint16_t pattern = cards_panels [card] [i] << 3;
            SMS_loadTiles (&cards_patterns [pattern], card_sprite [i], 32);
        }
    }
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
        /* The sprite attribute table only allows an 8-bit pattern index.
         * The cards use more than 256 patterns, so the particular card
         * we want to render needs to be copied to a lower VRAM address. */
        SMS_addSprite (x + (panel_x << 3),
                       y + (panel_y << 3), card_sprite [pattern_index++]);

    }
}


/*
 * Render a card to the background.
 * TODO: Temporarily taking in the slot number to simplify caching.
 */
void render_card_as_background (uint8_t x, uint8_t y, card_t card, uint8_t slot)
{
    /* Record of current patterns stored in the card buffer area */
    static uint8_t cache [9] = {
        CARD_NONE, CARD_NONE, CARD_NONE, CARD_NONE,
        CARD_NONE, CARD_NONE, CARD_NONE, CARD_NONE,
        CARD_NONE
    };

    if (card == CARD_NONE)
    {
        SMS_loadTileMapArea (x, y, empty_slot, 4, 6);
    }
    else if (card == CARD_BACK)
    {
        SMS_loadTileMapArea (x, y, card_back, 4, 6);
    }
    else
    {
        /* If the card is not the same card that was previously
         * in this buffer slot, then update the buffer. */
        if (cache [slot] != card)
        {
            cache [slot] = card;

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
        }

        SMS_loadTileMapArea (x, y, card_buffer [slot], 4, 6);
    }
}


/*
 * Prepare for the card sliding animation.
 * This will render the card as a sprite in the starting position.
 */

void card_slide_from (uint16_t start_x, uint16_t start_y, card_t card)
{
    slide_start_x = start_x;
    slide_start_y = start_y;

    SMS_initSprites ();
    render_card_as_sprite_prepare (card);
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
 * Entry point.
 */
void main (void)
{
    /* Setup */
    SMS_loadBGPalette (background_palette);
    SMS_loadSpritePalette (sprite_palette);
    SMS_setBackdropColor (0);

    /* Copy the static patterns into VRAM, but not the cards. There are too many
     * cards to fit in VRAM at once, so they will be loaded as needed. */
    SMS_loadTiles (blank_pattern, PATTERN_BLANK, sizeof (blank_pattern));

    player_patterns_start = STATIC_PATTERNS_START;
    SMS_loadTiles (player_patterns, player_patterns_start, sizeof (player_patterns));

    panel_patterns_start = player_patterns_start + (sizeof (player_patterns) >> 5);
    SMS_loadTiles (panel_patterns, panel_patterns_start, sizeof (panel_patterns));

    card_buffer_prepare ();
    clear_background ();

    SMS_useFirstHalfTilesforSprites (true);
    SMS_initSprites ();
    SMS_copySpritestoSAT ();

    SMS_displayOn ();

    sram_load ();
    rng_seed ();

    while (true)
    {
        game_start ();
    }
}

SMS_EMBED_SEGA_ROM_HEADER(9999, 0);
