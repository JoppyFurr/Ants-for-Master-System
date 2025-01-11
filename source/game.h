/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 *
 * Gameplay header
 */

typedef enum field_e {
    BUILDERS = 0,
    BRICKS,
    SOLDIERS,
    WEAPONS,
    MAGI,
    CRYSTALS,
    CASTLE,
    FENCE,
    FIELD_MAX
} field_t;

typedef struct card_data_s
{
    field_t cost_unit;
    uint8_t cost;
} card_data_t;

#ifdef INCLUDE_CARD_DATA
static const card_data_t card_data [30] = {
    [CARD_WALL]             = { .cost_unit = BRICKS,   .cost =  1 },
    [CARD_BASE]             = { .cost_unit = BRICKS,   .cost =  1 },
    [CARD_DEFENCE]          = { .cost_unit = BRICKS,   .cost =  3 },
    [CARD_RESERVE]          = { .cost_unit = BRICKS,   .cost =  3 },
    [CARD_TOWER]            = { .cost_unit = BRICKS,   .cost =  5 },
    [CARD_SCHOOL]           = { .cost_unit = BRICKS,   .cost =  8 },
    [CARD_WAIN]             = { .cost_unit = BRICKS,   .cost = 10 },
    [CARD_FENCE]            = { .cost_unit = BRICKS,   .cost = 12 },
    [CARD_FORT]             = { .cost_unit = BRICKS,   .cost = 18 },
    [CARD_BABYLON]          = { .cost_unit = BRICKS,   .cost = 39 },

    [CARD_ARCHER]           = { .cost_unit = WEAPONS,  .cost =  1 },
    [CARD_KNIGHT]           = { .cost_unit = WEAPONS,  .cost =  2 },
    [CARD_RIDER]            = { .cost_unit = WEAPONS,  .cost =  2 },
    [CARD_PLATOON]          = { .cost_unit = WEAPONS,  .cost =  4 },
    [CARD_RECRUIT]          = { .cost_unit = WEAPONS,  .cost =  8 },
    [CARD_ATTACK]           = { .cost_unit = WEAPONS,  .cost = 10 },
    [CARD_SABOTEUR]         = { .cost_unit = WEAPONS,  .cost = 12 },
    [CARD_THIEF]            = { .cost_unit = WEAPONS,  .cost = 15 },
    [CARD_SWAT]             = { .cost_unit = WEAPONS,  .cost = 18 },
    [CARD_BANSHEE]          = { .cost_unit = WEAPONS,  .cost = 28 },

    [CARD_CONJURE_BRICKS]   = { .cost_unit = CRYSTALS, .cost =  4 },
    [CARD_CRUSH_BRICKS]     = { .cost_unit = CRYSTALS, .cost =  4 },
    [CARD_CONJURE_WEAPONS]  = { .cost_unit = CRYSTALS, .cost =  4 },
    [CARD_CRUSH_WEAPONS]    = { .cost_unit = CRYSTALS, .cost =  4 },
    [CARD_CONJURE_CRYSTALS] = { .cost_unit = CRYSTALS, .cost =  4 },
    [CARD_CRUSH_CRYSTALS]   = { .cost_unit = CRYSTALS, .cost =  4 },
    [CARD_SORCERER]         = { .cost_unit = CRYSTALS, .cost =  8 },
    [CARD_DRAGON]           = { .cost_unit = CRYSTALS, .cost = 21 },
    [CARD_PIXIES]           = { .cost_unit = CRYSTALS, .cost = 22 },
    [CARD_CURSE]            = { .cost_unit = CRYSTALS, .cost = 25 }
};
#endif

/* Play one game of Ants. */
void game_start (void);
