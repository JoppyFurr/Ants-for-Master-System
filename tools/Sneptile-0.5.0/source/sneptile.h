/*
 * Sneptile
 * Joppy Furr 2024
 */

/* Return Codes */
#define RC_OK       0
#define RC_ERROR   -1

typedef struct pixel_s {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} pixel_t;

typedef enum target_e {
    VDP_MODE_0 = 0,
    VDP_MODE_2,
    VDP_MODE_TMS_SMALL_SPRITES,
    VDP_MODE_TMS_LARGE_SPRITES,
    VDP_MODE_4
} target_t;

/* Global State */
extern target_t target;
extern char *output_dir;

