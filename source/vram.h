/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 */

/*
 * Title screen VRAM layout:
 *   [  0..359] - Title image
 */
#define PATTERN_TITLE_IMAGE         0

/*
 * Gameplay VRAM Layout:
 *
 *   [       0] - Blank pattern
 *   [  1..240] - ten-card dynamic buffer
 *   [241..272] - Panel digit area
 *   [273..302] - Castle dynamic buffer (left)
 *   [303..332] - Castle dynamic buffer (right)
 *   [333..336] - Fence dynamic buffer (left)
 *   [337..340] - Fence dynamic buffer (right)
 *   [341..   ] - Static patterns (72 for now)
 */

#define PATTERN_BLANK               0
#define PATTERN_CARD_BUFFER         1
#define PATTERN_PANEL_DIGITS      241
#define PATTERN_CASTLE_1_BUFFER   273
#define PATTERN_CASTLE_2_BUFFER   303
#define PATTERN_FENCE_1_BUFFER    333
#define PATTERN_FENCE_2_BUFFER    337
#define STATIC_PATTERNS_START     341

