/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 */

/*
 * Title screen VRAM layout:
 *   [  0.. 11] - Cursor
 *   [ 12.. 23] - Widgets
 *   [ 24..427] - Title image
 */
#define PATTERN_CURSOR              0
#define PATTERN_WIDGETS            12
#define PATTERN_TITLE_IMAGE        24

/*
 * Gameplay VRAM Layout:
 *
 *   [       0] - Blank pattern
 *   [  1..240] - ten-card dynamic buffer
 *   [241..244] - Hand cursor
 *   [245..276] - Panel digit area
 *   [277..306] - Castle dynamic buffer (left)
 *   [307..336] - Castle dynamic buffer (right)
 *   [337..340] - Win-counter digits
 *   [341..356] - Player indicators
 *   [357..416] - Panels <-- Contains recoverable patterns where digits will go.
 *   [422..447] - Background, up against the edge of the name-table.
 *
 *   [341..344] - Fence dynamic buffer (left)
 *   [345..348] - Fence dynamic buffer (right)
 */

#define PATTERN_BLANK               0
#define PATTERN_CARD_BUFFER         1
#define PATTERN_HAND_CURSOR       241
#define PATTERN_PANEL_DIGITS      245
#define PATTERN_CASTLE_1_BUFFER   277
#define PATTERN_CASTLE_2_BUFFER   307
#define PATTERN_WIN_DIGITS        337
#define PATTERN_PLAYERS           341
#define PATTERN_PANELS            357
#define PATTERN_BACKGROUND        422

/* Because they're the correct size - The dynamic fence
 * patterns squeeze into the unused portion of the name-table. */
#define PATTERN_FENCE_1_BUFFER    496
#define PATTERN_FENCE_2_BUFFER    500

