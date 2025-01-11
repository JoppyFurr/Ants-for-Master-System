/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 *
 * Gameplay header
 */

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

/* Play one game of Ants. */
void game_start (void);
