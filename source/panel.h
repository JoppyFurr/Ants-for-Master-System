/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 *
 * Side-panel header
 */

/* Update the win counters at the top of the screen. */
void panel_update_wins (uint8_t player);

/* Initialise the win counter. */
void panel_init_wins (void);

/* Update the player indicator at the top of the screen. */
void panel_update_player (uint8_t player);

/* Initialise the side-panel. */
void panel_init (void);

/* Update the side-panel values. */
void panel_update (void);
