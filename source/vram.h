/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 */

/*
 * Title screen VRAM layout:
 *   [  0.. 11] - Cursor
 *   [ 12..409] - Title image
 *   [410..417] - Widgets
 */
#define PATTERN_CURSOR              0
#define PATTERN_TITLE_IMAGE        12
#define PATTERN_WIDGETS           410

/*
 * Gameplay VRAM Layout:
 *
 *   [       0] - Blank pattern
 *   [  1..240] - ten-card dynamic buffer
 *   [241..248] - Hand cursor
 *   [249..280] - Panel digit area
 *   [281..310] - Castle dynamic buffer (left)
 *   [311..340] - Castle dynamic buffer (right)
 *   [341..344] - Fence dynamic buffer (left)
 *   [345..348] - Fence dynamic buffer (right)
 *   [349..352] - Win-counter digits
 *   [353..   ] - Static patterns (72 for now)
 */

#define PATTERN_BLANK               0
#define PATTERN_CARD_BUFFER         1
#define PATTERN_HAND_CURSOR       241
#define PATTERN_PANEL_DIGITS      249
#define PATTERN_CASTLE_1_BUFFER   281
#define PATTERN_CASTLE_2_BUFFER   311
#define PATTERN_FENCE_1_BUFFER    341
#define PATTERN_FENCE_2_BUFFER    345
#define PATTERN_WIN_DIGITS        349
#define STATIC_PATTERNS_START     353

