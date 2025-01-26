/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 *
 * Card enums and data
 */

/* Card on-screen locations */
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

#define CARD_BITS   0x1f
#define DISCARD_BIT 0x80

typedef enum card_e
{
    CARD_WALL = 0,
    CARD_BASE,
    CARD_DEFENCE,
    CARD_RESERVE,
    CARD_TOWER,
    CARD_SCHOOL,
    CARD_WAIN,
    CARD_FENCE,
    CARD_FORT,
    CARD_BABYLON,

    CARD_ARCHER,
    CARD_KNIGHT,
    CARD_RIDER,
    CARD_PLATOON,
    CARD_RECRUIT,
    CARD_ATTACK,
    CARD_SABOTEUR,
    CARD_THIEF,
    CARD_SWAT,
    CARD_BANSHEE,

    CARD_CONJURE_BRICKS,
    CARD_CRUSH_BRICKS,
    CARD_CONJURE_WEAPONS,
    CARD_CRUSH_WEAPONS,
    CARD_CONJURE_CRYSTALS,
    CARD_CRUSH_CRYSTALS,
    CARD_SORCERER,
    CARD_DRAGON,
    CARD_PIXIES,
    CARD_CURSE,

    CARD_BACK,
    CARD_DISCARD,
    CARD_NONE
} card_t;
