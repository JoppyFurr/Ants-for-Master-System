/*
 * Sneptile
 * Joppy Furr 2024
 */

/* Open the three output files. */
int tms9928a_open_files (void);

/* Finalize and Close the three output files. */
int tms9928a_close_files (void);

/* Mark the start of a new source file. */
void tms9928a_new_input_file (const char *name);

/* Process a single tile. */
void tms9928a_process_tile (pixel_t *buffer, uint32_t stride);
