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
    VDP_MODE_4,
    VDP_MODE_4_SPRITES,
} target_t;

/* Global State */
extern target_t target;
extern char *output_dir;

/* Current image file */
typedef struct image_s {
    uint32_t width;
    uint32_t height;
} image_t;
extern image_t current_image;

/* Find the matching 8x8 tile, or -1 if it is unique. */
int32_t sneptile_get_match (pixel_t *tile);
