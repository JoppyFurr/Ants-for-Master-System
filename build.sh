#!/bin/sh
echo ""
echo "Ants for Master System Build Script"
echo "-----------------------------------"

sdcc="${HOME}/Code/sdcc-4.3.0/bin/sdcc"
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

    # Cards use the sprite palette, as they can slide across the screen. However,
    # the card artwork is large enough to need its own bank which is currently
    # implemented as a separate call to Sneptile. To keep the palette consistent
    # for both banks of in-game patterns, the palette is defined here.
    # Note that the first three entries are shared with the title sprite palette,
    # so that the same cursor patterns can be used.
    # Sprite palette [0] is black (transparent)
    # Sprite palette [1] is black (cursor)
    # Sprite palette [2] is white (cursor)
    # Sprite palette [3] is red (disabled cursor)
    # Sprite palette [4] is green (win counter digits)
    GAME_SPRITE_PALETTE="0x00 0x00 0x3f 0x02 0x08 0x15 0x2a 0x03 0x3a 0x25 0x2b 0x06 0x1b 0x2e 0x21 0x34"


    echo "  Generating tile data..."
    mkdir -p title_tile_data
    (
        # Sprite palette [0] is black (transparent)
        # Sprite palette [1] is black (cursor)
        # Sprite palette [2] is white (cursor)
        # Sprite palette [3] is red (disabled cursor)
        # Background palette [0] is black (background colour)
        ${sneptile} --sprites --output-dir title_tile_data \
            --sprite-palette 0x00 0x00 0x3f 0x02 \
            --background-palette 0x00 \
            --panels 2x2,3 tiles/cursor.png \
            --background tiles/title.png \
            --background --panels 2x1,4 tiles/widgets.png
    )
    mkdir -p game_tile_data
    (
        # Background palette [0] is black (background colour)
        # Background palette [1] is white (digit printing)
        # Background palette [2] is yellow (digit printing)
        ${sneptile} --sprites --output-dir game_tile_data \
            --sprite-palette ${GAME_SPRITE_PALETTE} \
            --background-palette 0x00 0x3f 0x1f \
            --panels 4x2,4 tiles/player.png \
            --background tiles/panel.png \
            --panels 6x3,2 tiles/castles.png \
            --panels 1x2,2 tiles/fence.png
    )
    mkdir -p card_tile_data
    (
        ${sneptile} --sprites --output-dir card_tile_data \
            --sprite-palette ${GAME_SPRITE_PALETTE} \
            --panels 4x6,32 tiles/cards.png
    )

    mkdir -p sound_data
    echo "  Generating sound data..."
    for sound in card build-castle ruin-castle build-fence ruin-fence \
                 increase-stocks decrease-stocks increase-power curse
    do
        # Don't process sounds that are already up to date
        if [ -e "./sound_data/${sound}.h" -a "./sounds/${sound}.wav" -ot "./sound_data/${sound}.h" ]
        then
            continue
        fi

        # Convert with pcmenc, then store the sound data in a C header file.
        ${pcmenc} -rto 1 -dt1 12 -dt2 12 -dt3 423 "./sounds/${sound}.wav"
        mv "./sounds/${sound}.wav.pcmenc" "./sound_data/${sound}_sound"
        (
            cd "./sound_data/"
            xxd --include "${sound}_sound" > ${sound}.h
            sed -e "s/unsigned char/const uint8_t/" \
                -e "/unsigned int/d" \
                --in-place ${sound}.h
        )
        rm "./sound_data/${sound}_sound"
    done

    # Fanfare doesn't fit in a single bank, so split it
    for sound in fanfare
    do
        # Don't process sounds that are already up to date
        if [ -e "./sound_data/${sound}_1.h" -a "./sounds/${sound}.wav" -ot "./sound_data/${sound}_1.h" ]
        then
            continue
        fi

        # Convert with pcmenc, splitting to 16 KiB of sample data per C header file.
        ${pcmenc} -rto 1 -dt1 12 -dt2 12 -dt3 423 "./sounds/fanfare.wav" -r 16
        mv "./sounds/${sound}.wav.pcmenc" "./sound_data/${sound}_sound"
        (
            cd "./sound_data/"
            xxd --include -l 16384 -n "${sound}_sound_1" "${sound}_sound" > ${sound}_1.h
            xxd --include -s 16384 -n "${sound}_sound_2" "${sound}_sound" > ${sound}_2.h
            sed -e "s/unsigned char/const uint8_t/" \
                -e "/unsigned int/d" \
                --in-place ${sound}_1.h \
                --in-place ${sound}_2.h
        )
        rm "./sound_data/${sound}_sound"
    done

    mkdir -p build/code
    echo "  Compiling..."
    for file in main title game castle panel sound rng save
    do
        # Don't recompile files that are already up to date
        if [ -e "./build/code/${file}.rel" -a "./source/${file}.c" -ot "./build/code/${file}.rel" ]
        then
            continue
        fi

        echo "   -> ${file}.c"
        ${sdcc} -c -mz80 --peep-file ${devkitSMS}/SMSlib/src/peep-rules.txt -I ${SMSlib}/src \
            -o "build/code/${file}.rel" "source/${file}.c" || exit 1
    done

    # Libraries
    ${sdcc} -c -mz80 --peep-file ${devkitSMS}/SMSlib/src/peep-rules.txt -I ${SMSlib}/src \
        -o "build/code/fxsample.rel" "libraries/sms-fxsample/fxsample.c" || exit 1

    # Asset banks
    for bank in 2 3 4 5 6 7 8 9 10 11
    do
        ${sdcc} -c -mz80 --constseg BANK_${bank} source/bank_${bank}.c -o build/bank_${bank}.rel
    done

    echo ""
    echo "  Linking..."
    ${sdcc} -o build/Ants.ihx -mz80 --no-std-crt0 --data-loc 0xC000 \
        -Wl-b_BANK_2=0x8000 -Wl-b_BANK_3=0x8000 -Wl-b_BANK_4=0x8000 -Wl-b_BANK_5=0x8000 \
        -Wl-b_BANK_6=0x8000 -Wl-b_BANK_7=0x8000 -Wl-b_BANK_8=0x8000 -Wl-b_BANK_9=0x8000 \
        -Wl-b_BANK_10=0x8000 -Wl-b_BANK_11=0x8000 \
        ${devkitSMS}/crt0/crt0_sms.rel \
        build/code/*.rel \
        ${SMSlib}/SMSlib.lib \
        build/bank_2.rel build/bank_3.rel build/bank_4.rel build/bank_5.rel \
        build/bank_6.rel build/bank_7.rel build/bank_8.rel build/bank_9.rel \
        build/bank_10.rel build/bank_11.rel || exit 1

    echo ""
    echo "  Generating ROM..."
    ${ihx2sms} build/Ants.ihx Ants.sms || exit 1

    echo ""
    echo "  Done"
}

# Normal build:
#   Recompile all code, but don't re-generate the
#   audio samples unless the source .wav files have
#   changed.
#
# Clean build:
#   Regenerate everything.
#
# Fast build:
#   Only rebuild source files that have changed.
#   Changes to tiles or header files will not trigger source rebuilds.
if [ "${1}" = "clean" ]
then
    rm -rf title_tile_data
    rm -rf game_tile_data
    rm -rf card_tile_data
    rm -rf sound_data
    rm -rf build
elif [ "${1}" != "fast" ]
then
    rm -rf build
fi

build_pcmenc
build_sneptile
build_ants_for_master_system
