
# Sneptile
Sneptile is a tool for converting images into tile data for the Sega Master System.

Input images should have a width and height that are multiples of 8px.
Tiles are generated left-to-right, top-to-bottom, first file to last file.

Usage: `./Sneptile [--mode-0] --output tile_data --palette 0x04 0x19 [--reserve name,count] empty.png cursor.png`

 * `--mode-0`: Generate Mode-0 tiles.
 * `--mode-2`: Generate Mode-2 tiles.
 * `--tms-small-sprites`: Generate 8x8 sprites for the TMS modes.
 * `--tms-large-sprites`: Generate 16x16 sprites for the TMS modes.
 * `--sprites`: Mode-4 sprites. Index 0 will not be used for visible colours.
 * `--de-duplicate`: Within an input file, don't generate the same pattern twice.
 * `--output <dir>`: specifies the directory for the generated files
 * `--palette <0x...>`: specifies the first n entries of the palette
 * `--reserve <name,n>`: Reserve <n> patterns before the next sheet (eg, for runtime generated patterns).
 * `--background`: The next sheet should use the background palette instead of the default sprite palette (mode-4)
 * `--panels <wxh,n>`: Per-image, describes <n> panels of size <w> x <h> tiles. Mode-4 only, and depends on de-duplication.
 * `... <.png>`: the remaining parameters are `.png` images to generate tiles from

The following three files are generated in the specified output directory:

patterns.h contains the pattern data to load into the VDP:
```
const uint32_t patterns [] = {

    /* empty.png */
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,

    /* cursor.png */
    0x0000c000, 0x0000e040, 0x0000f060, 0x0000f870, 0x0000fc78, 0x0000fe7c, 0x0000ff7e, 0x0000ff7f,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00008000,
    0x0000ff7f, 0x0000ff7c, 0x0000fe6c, 0x0000ef46, 0x0000cf06, 0x00008703, 0x00000703, 0x00000300,
    0x0000c080, 0x0000e000, 0x00000000, 0x00000000, 0x00000000, 0x00008000, 0x00008000, 0x00008000,
    0x00008080, 0x00804040, 0x00c02060, 0x00e01070, 0x00b04838, 0x00b8443c, 0x009c621e, 0x009e611f,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x008f700f, 0x00906f1f, 0x00a45a36, 0x00ca2563, 0x008a4543, 0x00058281, 0x00070003, 0x00000303,
    0x00008080, 0x0000c0c0, 0x00000000, 0x00000000, 0x00000000, 0x00008080, 0x00008080, 0x00008080,
    0x0000c0c0, 0x0000e0a0, 0x0000f090, 0x0000f888, 0x0000fc84, 0x0000fe82, 0x0000ff81, 0x0000ff80,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00008080,
    0x0000ff80, 0x0000ff83, 0x0000fe92, 0x0000efa9, 0x0000cfc9, 0x00008784, 0x00000704, 0x00000303,
    0x0000c040, 0x0000e0e0, 0x00000000, 0x00000000, 0x00000000, 0x00008080, 0x00008080, 0x00008080,
};
```

pattern_index.h contains the index of the first tile from each image file:
```
#define PATTERN_EMPTY 0
#define PATTERN_CURSOR 1
```

palette.h contains the palette:
```
#ifdef TARGET_SMS
static const uint8_t palette [16] = { 0x04, 0x19, 0x3f, 0x00, 0x15, 0x2a };
#elif defined (TARGET_GG)
static const uint16_t palette [16] = { 0x0050, 0x05a5, 0x0fff, 0x0000, 0x0555, 0x0aaa };
#endif
```

Note that while only the Master System's 64 colours are supported, the generated palette
is available both in 6-bit Master System format, and a 12-bit Game Gear format, to allow
re-use on the Game Gear.

To select the correct palette, you will need to define one of `TARGET_SMS` or `TARGET_GG`.

## Panels
Per-file, a panel size and count can be described. When used an array of indexes will
be generated in pattern_index.h.

An example showing two files.
The first file containing a single tile, the second containing 31 playing card panels:
```
./Sneptile --de-duplicate --sprites --palette 0x00 \
    tiles/empty.png \
    --panels 4x6,31 \
    tiles/cards.png

```
will generate the following pattern_index.h:
```
#define PATTERN_EMPTY 0
#define PATTERN_CARDS 1
uint16_t panel_cards [31] [24] = {
    { 0x0829, 0x082a, 0x082b, 0x082c, 0x0835, 0x0836, 0x0837, 0x0838, 0x0835, 0x0856, 0x0856, 0x0838,
      0x086b, 0x086c, 0x086d, 0x086e, 0x088c, 0x088d, 0x088d, 0x088e, 0x0898, 0x0899, 0x0899, 0x089a },
    { 0x0829, 0x082a, 0x082b, 0x082c, 0x0835, 0x0839, 0x083a, 0x0838, 0x0835, 0x0856, 0x0856, 0x0838,
      0x086f, 0x0870, 0x0871, 0x0872, 0x088c, 0x088d, 0x088d, 0x088e, 0x0898, 0x0899, 0x0899, 0x089a },
    ...
    { 0x08ea, 0x08eb, 0x08ef, 0x08f2, 0x08f7, 0x0907, 0x0908, 0x08fa, 0x0918, 0x091b, 0x091c, 0x08fa,
      0x092b, 0x092c, 0x092d, 0x092e, 0x0918, 0x093c, 0x093d, 0x08fa, 0x093e, 0x093f, 0x093f, 0x0940 },
    { 0x0941, 0x0942, 0x0943, 0x0944, 0x0946, 0x0947, 0x0948, 0x0949, 0x094a, 0x094b, 0x094c, 0x094d,
      0x094e, 0x094f, 0x0950, 0x0951, 0x0952, 0x0953, 0x0954, 0x0955, 0x0956, 0x0957, 0x0958, 0x0959 }
};
```

Bit 11 indicates that the sprite palette is being used (default).

Note that, for now: --panels will only work when using mode-4 and de-duplication.

## TMS99xx Mode-0 and Mode-2

Initial support is also available for Mode-0 and Mode-2 of the TMS9918 family.

Three files are output:
 * `patterns.h`: Contains the pattern data to load into the VDP.
 * `pattern_index.h`: Contains the index of the first tile from each image file.
 * `colour_table.h`: Contains the colour table to load into the VDP.

Note that, in Mode-0, groups of eight tiles in the pattern table are required
to share a common two-colour entry in the colour table.

If these two colours change between images, or between tiles within the same
image, then all-zero tiles will be added to pad out the remaining tiles in the
block of eight.

To keep offsets from the defines in `pattern_index.h` useful, it is recommended
to use only two colours per file.

The input files should use the gamma-corrected palette:
```c
/* TMS9928a palette (gamma corrected) */
static const pixel_t tms9928a_palette [16] = {
    { .r = 0x00, .g = 0x00, .b = 0x00 },    /* Transparent */
    { .r = 0x00, .g = 0x00, .b = 0x00 },    /* Black */
    { .r = 0x0a, .g = 0xad, .b = 0x1e },    /* Medium Green */
    { .r = 0x34, .g = 0xc8, .b = 0x4c },    /* Light Green */
    { .r = 0x2b, .g = 0x2d, .b = 0xe3 },    /* Dark Blue */
    { .r = 0x51, .g = 0x4b, .b = 0xfb },    /* Light Blue */
    { .r = 0xbd, .g = 0x29, .b = 0x25 },    /* Dark Red */
    { .r = 0x1e, .g = 0xe2, .b = 0xef },    /* Cyan */
    { .r = 0xfb, .g = 0x2c, .b = 0x2b },    /* Medium Red */
    { .r = 0xff, .g = 0x5f, .b = 0x4c },    /* Light Red */
    { .r = 0xbd, .g = 0xa2, .b = 0x2b },    /* Dark Yellow */
    { .r = 0xd7, .g = 0xb4, .b = 0x54 },    /* Light Yellow */
    { .r = 0x0a, .g = 0x8c, .b = 0x18 },    /* Dark Green */
    { .r = 0xaf, .g = 0x32, .b = 0x9a },    /* Magenta */
    { .r = 0xb2, .g = 0xb2, .b = 0xb2 },    /* Grey */
    { .r = 0xff, .g = 0xff, .b = 0xff }     /* White */
};
```

## TMS99xx Sprites
Sprites are generated when using `--tms-small-sprites` or `--tms-large-sprites`.
The output files:
 * For 8x8 sprites: `sprites.h` and `sprite_index.h`
 * For 16x16 sprites: `sprites_l.h` and `sprite_index_l.h`

As each sprite is only a single colour configured in the attribute table:
 * Any transparent pixel is set to a `0` in the sprite bitmap.
 * Any non-transparent pixel is set to a `1` in the sprite bitmap.

## Dependencies
 * zlib
