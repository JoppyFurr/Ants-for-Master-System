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
    CARD_NONE
} card_t;

typedef enum card_type_e
{
    CARD_TYPE_BUILDING = 0,
    CARD_TYPE_ARMY,
    CARD_TYPE_MAGIC
} card_type_t;

typedef struct card_data_s
{
    uint8_t type;
    uint8_t cost;
} card_data_t;

#ifdef INCLUDE_CARD_DATA
static const card_data_t card_data [30] = {
    [CARD_WALL]             = { .type = CARD_TYPE_BUILDING, .cost =  1 },
    [CARD_BASE]             = { .type = CARD_TYPE_BUILDING, .cost =  1 },
    [CARD_DEFENCE]          = { .type = CARD_TYPE_BUILDING, .cost =  3 },
    [CARD_RESERVE]          = { .type = CARD_TYPE_BUILDING, .cost =  3 },
    [CARD_TOWER]            = { .type = CARD_TYPE_BUILDING, .cost =  5 },
    [CARD_SCHOOL]           = { .type = CARD_TYPE_BUILDING, .cost =  8 },
    [CARD_WAIN]             = { .type = CARD_TYPE_BUILDING, .cost = 10 },
    [CARD_FENCE]            = { .type = CARD_TYPE_BUILDING, .cost = 12 },
    [CARD_FORT]             = { .type = CARD_TYPE_BUILDING, .cost = 18 },
    [CARD_BABYLON]          = { .type = CARD_TYPE_BUILDING, .cost = 39 },

    [CARD_ARCHER]           = { .type = CARD_TYPE_ARMY,     .cost =  1 },
    [CARD_KNIGHT]           = { .type = CARD_TYPE_ARMY,     .cost =  2 },
    [CARD_RIDER]            = { .type = CARD_TYPE_ARMY,     .cost =  2 },
    [CARD_PLATOON]          = { .type = CARD_TYPE_ARMY,     .cost =  4 },
    [CARD_RECRUIT]          = { .type = CARD_TYPE_ARMY,     .cost =  8 },
    [CARD_ATTACK]           = { .type = CARD_TYPE_ARMY,     .cost = 10 },
    [CARD_SABOTEUR]         = { .type = CARD_TYPE_ARMY,     .cost = 12 },
    [CARD_THIEF]            = { .type = CARD_TYPE_ARMY,     .cost = 15 },
    [CARD_SWAT]             = { .type = CARD_TYPE_ARMY,     .cost = 18 },
    [CARD_BANSHEE]          = { .type = CARD_TYPE_ARMY,     .cost = 28 },

    [CARD_CONJURE_BRICKS]   = { .type = CARD_TYPE_MAGIC,    .cost =  4 },
    [CARD_CRUSH_BRICKS]     = { .type = CARD_TYPE_MAGIC,    .cost =  4 },
    [CARD_CONJURE_WEAPONS]  = { .type = CARD_TYPE_MAGIC,    .cost =  4 },
    [CARD_CRUSH_WEAPONS]    = { .type = CARD_TYPE_MAGIC,    .cost =  4 },
    [CARD_CONJURE_CRYSTALS] = { .type = CARD_TYPE_MAGIC,    .cost =  4 },
    [CARD_CRUSH_CRYSTALS]   = { .type = CARD_TYPE_MAGIC,    .cost =  4 },
    [CARD_SORCERER]         = { .type = CARD_TYPE_MAGIC,    .cost =  8 },
    [CARD_DRAGON]           = { .type = CARD_TYPE_MAGIC,    .cost = 21 },
    [CARD_PIXIES]           = { .type = CARD_TYPE_MAGIC,    .cost = 22 },
    [CARD_CURSE]            = { .type = CARD_TYPE_MAGIC,    .cost = 25 }
};
#endif
