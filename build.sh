#!/bin/sh
echo ""
echo "Ants for Master System Build Script"
echo "-----------------------------------"

sdcc="${HOME}/Code/sdcc-4.2.0/bin/sdcc"
devkitSMS="${HOME}/Code/devkitSMS"
SMSlib="${devkitSMS}/SMSlib"
ihx2sms="${devkitSMS}/ihx2sms/Linux/ihx2sms"
pcmenc="./tools/pcmenc/encoder/pcmenc"
sneptile="./tools/Sneptile-0.10.0/Sneptile"

build_pcmenc ()
{
    # Early return if we've already got an up-to-date build
    if [ -e $pcmenc -a "./tools/pcmenc/encoder/pcmenc.cpp" -ot $pcmenc ]
    then
        return
    fi

    echo "Building pcmenc..."
    (
        cd "tools/pcmenc/encoder"
        make
        echo ""
    )
}

build_sneptile ()
{
    # Early return if we've already got an up-to-date build
    if [ -e $sneptile -a "./tools/Sneptile-0.10.0/source/main.c" -ot $sneptile ]
    then
        return
    fi

    echo "Building Sneptile..."
    (
        cd "tools/Sneptile-0.10.0"
        ./build.sh
    )
}

build_ants_for_master_system ()
{
    echo "Building Ants for Master System..."

    echo "  Generating tile data..."
    mkdir -p tile_data
    (
        # Sprite palette [0] is black (background colour)
        # Background palette [0] is black (background colour)
        # Background palette [1] is white (digit printing)
        # Background palette [2] is yellow (digit printing)
        $sneptile --sprites --output-dir tile_data \
            --background-palette 0x00 0x3f 0x1f \
            --sprite-palette 0x00 \
            --panels 4x2,4 tiles/player.png \
            --background tiles/panel.png \
            --panels 6x3,2 tiles/castles.png \
            --panels 1x2,2 tiles/fence.png \
            --panels 4x6,32 tiles/cards.png

    )

    mkdir -p build/code
    echo "  Compiling..."
    for file in main game castle panel rng save
    do
        echo "   -> ${file}.c"
        ${sdcc} -c -mz80 --peep-file ${devkitSMS}/SMSlib/src/peep-rules.txt -I ${SMSlib}/src \
            -o "build/code/${file}.rel" "source/${file}.c" || exit 1
    done

    # Asset banks
    ${sdcc} -c -mz80 --constseg BANK_2 source/bank_2.c -o build/bank_2.rel

    echo ""
    echo "  Linking..."
    ${sdcc} -o build/Ants.ihx -mz80 --no-std-crt0 --data-loc 0xC000 -Wl-b_BANK_2=0x8000 \
        ${devkitSMS}/crt0/crt0_sms.rel \
        build/code/*.rel \
        ${SMSlib}/SMSlib.lib \
        build/bank_2.rel || exit 1

    echo ""
    echo "  Generating ROM..."
    ${ihx2sms} build/Ants.ihx Ants.sms || exit 1

    echo ""
    echo "  Done"
}

# Clean up any old build artefacts
rm -rf build

build_pcmenc
build_sneptile
build_ants_for_master_system
