/*
 * Ants for Master System
 *
 * An Ants clone for the Sega Master System
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "SMSlib.h"

#define TARGET_SMS
#include "../tile_data/palette.h"
#include "../tile_data/patterns.h"
#include "../tile_data/pattern_index.h"
#include "digits.h"

#include "cards.h"
#include "save.h"
#include "rng.h"

#define HAND_Y_TILE 18
#define HAND_Y_SPRITE 144

#define DRAW_X_TILE 12
#define DRAW_Y_TILE 0
#define DRAW_X_SPRITE 96
#define DRAW_Y_SPRITE 0

#define DISCARD_X_TILE 16
#define DISCARD_Y_TILE 0
#define DISCARD_X_SPRITE 128
#define DISCARD_Y_SPRITE 0

typedef enum field_e {
    P1_BUILDERS = 0,
    P1_BRICKS,
    P1_SOLDIERS,
    P1_WEAPONS,
    P1_MAGI,
    P1_CRYSTALS,
    P1_CASTLE,
    P1_FENCE,
    P2_BUILDERS,
    P2_BRICKS,
    P2_SOLDIERS,
    P2_WEAPONS,
    P2_MAGI,
    P2_CRYSTALS,
    P2_CASTLE,
    P2_FENCE,
    FIELD_MAX
} field_t;

/* Game State */
uint8_t player = 0;
card_t hands [2] [8];
uint16_t resources [FIELD_MAX];

/* Starting resources */
const uint16_t starting_resources [8] = {
    [P1_BUILDERS] = 2,
    [P1_BRICKS]   = 5,
    [P1_SOLDIERS] = 2,
    [P1_WEAPONS]  = 5,
    [P1_MAGI]     = 2,
    [P1_CRYSTALS] = 5,
    [P1_CASTLE]   = 30,
    [P1_FENCE]    = 10
};

/* Reserved patterns for card sprite use */
static const uint8_t card_sprite [24] =  {
    PATTERN_CARD_SPRITE +  0, PATTERN_CARD_SPRITE +  1, PATTERN_CARD_SPRITE +  2, PATTERN_CARD_SPRITE +  3,
    PATTERN_CARD_SPRITE +  4, PATTERN_CARD_SPRITE +  5, PATTERN_CARD_SPRITE +  6, PATTERN_CARD_SPRITE +  7,
    PATTERN_CARD_SPRITE +  8, PATTERN_CARD_SPRITE +  9, PATTERN_CARD_SPRITE + 10, PATTERN_CARD_SPRITE + 11,
    PATTERN_CARD_SPRITE + 12, PATTERN_CARD_SPRITE + 13, PATTERN_CARD_SPRITE + 14, PATTERN_CARD_SPRITE + 15,
    PATTERN_CARD_SPRITE + 16, PATTERN_CARD_SPRITE + 17, PATTERN_CARD_SPRITE + 18, PATTERN_CARD_SPRITE + 19,
    PATTERN_CARD_SPRITE + 20, PATTERN_CARD_SPRITE + 21, PATTERN_CARD_SPRITE + 22, PATTERN_CARD_SPRITE + 23
};

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
    for (uint8_t i = 0; i < 24; i++)
    {
        uint16_t pattern = panel_cards [card] [i] << 3;
        SMS_loadTiles (&patterns [pattern], card_sprite [i], 32);
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
        /* TODO: The sprite attribute table only allows an 8-bit pattern index.
         *       The cards use more than 256 patterns, so the particular card
         *       we want to render may need to be copied to a lower VRAM address. */
        SMS_addSprite (x + (panel_x << 3),
                       y + (panel_y << 3), card_sprite [pattern_index++]);

    }
}


/*
 * Clear a card from the background.
 */
static inline void clear_card_from_tile (uint8_t x, uint8_t y)
{
    uint16_t empty_slot [24] = {
        PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY,
        PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY,
        PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY,
        PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY,
        PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY,
        PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY, PATTERN_EMPTY
    };
    SMS_loadTileMapArea (x, y, empty_slot, 4, 6);
}


/*
 * Render a card to the background.
 */
static inline void render_card_as_tile (uint8_t x, uint8_t y, card_t card)
{
    SMS_loadTileMapArea (x, y, panel_cards [card], 4, 6);
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
 * Draw a card and place it into the active player's hand.
 */
void draw_card (uint8_t slot, bool hidden)
{
    /* Deal */
    card_t card = rand () % 30;
    hands [player] [slot] = card;

    /* Animate */
    if (hidden)
    {
        card = CARD_BACK;
    }

    card_slide_from (DRAW_X_SPRITE, DRAW_Y_SPRITE, card);
    card_slide_to (slot << 5, HAND_Y_SPRITE);
    render_card_as_tile (slot << 2, HAND_Y_TILE, card);
    card_slide_done ();
}


/*
 * Play a card from the active player's hand.
 */
void play_card (uint8_t slot)
{
    card_t card = hands [player] [slot];

    /* Animate */
    card_slide_from (slot << 5, HAND_Y_SPRITE, card);
    clear_card_from_tile (slot << 2, HAND_Y_TILE);
    card_slide_to (DISCARD_X_SPRITE, DISCARD_Y_SPRITE);
    render_card_as_tile (DISCARD_X_TILE, DISCARD_Y_TILE, card);
    card_slide_done ();
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
 * Initialise the side-panel.
 */
void panel_init (void)
{
    const uint8_t y_positions [8] = { 4, 5, 7, 8, 10, 11, 14, 15 };

    /* Copy tilemap from image */
    SMS_loadTileMapArea (0, 3, panel_panel [0], 4, 14);
    SMS_loadTileMapArea (28, 3, panel_panel [0], 4, 14);

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
 * Update a side-panel value.
 * TODO: Consider caching values and automatically updating all that
 *       have changed rather than requiring a 'field' parameter.
 */
void panel_update (field_t field)
{
    uint16_t value = resources [field];

    /* A pair of buffers to hold the two patterns we will draw into */
    uint8_t buffer_l [32];
    uint8_t buffer_r [32];

    /* Separate the digits. Assume a maximum value of 999. */
    uint8_t digit_0 = value % 10;           /* Ones */
    uint8_t digit_1 = (value % 100) / 10;   /* Tens */
    uint8_t digit_2 = value / 100;          /* Hundreds */

    /* Within each panel-box, the digit for the top
     * field is one pixel higher within its tile */
    uint8_t y_offset = (field & 0x01) ? 2 : 1;

    /* TODO: Bake text colours into background palette */
    /* Currently, Yellow (15) is missing as it's not in the image.
     *            White (63) happens to be at index 10 */
    const uint8_t colour_index = 10; /* White */

    /* Start by populating the pattern buffer with the panel background tiles */
    const uint8_t field_backgrounds [8] = { 6, 10, 18, 22, 30, 34, 46, 50 };
    memcpy (buffer_l,  &patterns [panel_panel [0] [field_backgrounds [field & 0x07] + 0] << 3], 32);
    memcpy (buffer_r, &patterns [panel_panel [0] [field_backgrounds [field & 0x07] + 1] << 3], 32);

    /* Draw the font into the pattern */
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

    /* TODO: Consider waiting for vsync before writing to VRAM.
     *       In some cases however (eg, curse) we will update all 16 fields
     *       at once and may want them to share a vsync if possible.. */
    SMS_loadTiles (buffer_l, PATTERN_PANEL_DIGITS + (field << 1),     sizeof (buffer_l));
    SMS_loadTiles (buffer_r, PATTERN_PANEL_DIGITS + (field << 1) + 1, sizeof (buffer_r));
}


/*
 * Change the current player.
 */
void set_player (uint8_t p)
{
    player = p;
    card_t *hand = hands [player];

    /* Player indicator */
    SMS_loadTileMapArea ( 0, 1, panel_player [0 | (player == 0 ? 2 : 0)], 4, 2);
    SMS_loadTileMapArea (28, 1, panel_player [1 | (player == 1 ? 2 : 0)], 4, 2);

    /* Show the new player's hand */
    for (uint8_t slot = 0; slot < 8; slot++)
    {
        if (hand [slot] == CARD_NONE)
        {
            clear_card_from_tile (slot << 2, HAND_Y_TILE);
        }
        else if (player == 0)
        {
            render_card_as_tile (slot << 2, HAND_Y_TILE, hand [slot]);
        }
        else
        {
            render_card_as_tile (slot << 2, HAND_Y_TILE, CARD_BACK);
        }
    }
}


/*
 * Generate resources for the current player.
 */
void generate_resources (void)
{
    if (player == 0)
    {
        resources [P1_BRICKS]   += resources [P1_BUILDERS];
        resources [P1_WEAPONS]  += resources [P1_SOLDIERS];
        resources [P1_CRYSTALS] += resources [P1_MAGI];
        panel_update (P1_BRICKS);
        panel_update (P1_WEAPONS);
        panel_update (P1_CRYSTALS);
    }
    else
    {
        resources [P2_BRICKS]   += resources [P2_BUILDERS];
        resources [P2_WEAPONS]  += resources [P2_SOLDIERS];
        resources [P2_CRYSTALS] += resources [P2_MAGI];
        panel_update (P2_BRICKS);
        panel_update (P2_WEAPONS);
        panel_update (P2_CRYSTALS);
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

    SMS_loadTiles (patterns, 0, sizeof (patterns));
    clear_background ();

    SMS_useFirstHalfTilesforSprites (true);
    SMS_initSprites ();
    SMS_copySpritestoSAT ();

    SMS_displayOn ();

    sram_load ();
    rng_seed ();

    /* Draw player indicator */
    SMS_loadTileMapArea (0, 1, panel_player [2], 4, 2);
    SMS_loadTileMapArea (28, 1, panel_player [1], 4, 2);

    /* Initialise side panels */
    panel_init ();
    memcpy (&resources [0], starting_resources, sizeof (starting_resources));
    memcpy (&resources [8], starting_resources, sizeof (starting_resources));
    for (uint8_t field = 0; field < FIELD_MAX; field++)
    {
        panel_update (field);
    }

    /* Draw / discard area */
    render_card_as_tile (12, 0, CARD_BACK);
    render_card_as_tile (16, 0, CARD_RESERVE);

    /* Clear hands */
    memset (hands [0], CARD_NONE, sizeof (hands [0]));
    memset (hands [1], CARD_NONE, sizeof (hands [1]));

    /* Deal Player 1 */
    set_player (0);
    for (uint8_t i = 0; i < 8; i++)
    {
        draw_card (i, false);
    }
    delay_frames (60);

    /* Deal Player 2 */
    set_player (1);
    for (uint8_t i = 0; i < 8; i++)
    {
        draw_card (i, true);
    }
    delay_frames (60);

    /* The player who starts does not generate
     * resources as their first turn begins. */
    bool generate = false;

    while (1)
    {
        /* Change player */
        set_player (!player);
        if (generate)
        {
            generate_resources ();
        }
        delay_frames (60);

        uint8_t move = rand () & 0x07;
        play_card (move);
        delay_frames (30);

        draw_card (move, (player == 1));
        delay_frames (30);

        generate = true;
    }
}

SMS_EMBED_SEGA_ROM_HEADER(9999, 0);
