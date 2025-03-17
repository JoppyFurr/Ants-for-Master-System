/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 */

/*
 * Title screen VRAM layout:
 *   [  0..  7] - Cursor
 *   [  8..405] - Title image
 *   [406..413] - Widgets
 */
#define PATTERN_CURSOR              0
#define PATTERN_TITLE_IMAGE         8
#define PATTERN_WIDGETS           406

/*
 * Gameplay VRAM Layout:
 *
 *   [       0] - Blank pattern
 *   [  1..240] - ten-card dynamic buffer
 *   [241..244] - Hand cursor
 *   [245..276] - Panel digit area
 *   [277..306] - Castle dynamic buffer (left)
 *   [307..336] - Castle dynamic buffer (right)
 *   [337..340] - Fence dynamic buffer (left)
 *   [341..344] - Fence dynamic buffer (right)
 *   [345..   ] - Static patterns (72 for now)
 */

#define PATTERN_BLANK               0
#define PATTERN_CARD_BUFFER         1
#define PATTERN_HAND_CURSOR       241
#define PATTERN_PANEL_DIGITS      245
#define PATTERN_CASTLE_1_BUFFER   277
#define PATTERN_CASTLE_2_BUFFER   307
#define PATTERN_FENCE_1_BUFFER    337
#define PATTERN_FENCE_2_BUFFER    341
#define STATIC_PATTERNS_START     345

