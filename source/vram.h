/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 *
 * VRAM Layout:
 *
 *   [       0] - Blank pattern
 *   [  1.. 24] - Card as sprite
 *   [ 25.. 48] - Card back
 *   [ 49..264] - nine-card dynamic buffer
 *   [265..296] - Panel digit area
 *   [297..   ] - Static patterns
 */

#define PATTERN_BLANK           0
#define PATTERN_CARD_SPRITE     1
#define PATTERN_CARD_BACK      25
#define PATTERN_CARD_BUFFER    49
#define PATTERN_PANEL_DIGITS  265
#define STATIC_PATTERNS_START  297

