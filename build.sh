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

    echo "  Generating tile data..."
    mkdir -p title_tile_data
    (
        # Sprite palette [0] is black (background colour)
        # Background palette [0] is black (background colour)
        ${sneptile} --sprites --output-dir title_tile_data \
            --background-palette 0x00 \
            --sprite-palette 0x00 \
            --panels 2x2,2 tiles/cursor.png \
            --background tiles/title.png \
            --background --panels 2x1,4 tiles/widgets.png
    )
    mkdir -p game_tile_data
    (
        # Sprite palette [0] is black (background colour)
        # Background palette [0] is black (background colour)
        # Background palette [1] is white (digit printing)
        # Background palette [2] is yellow (digit printing)
        ${sneptile} --sprites --output-dir game_tile_data \
            --background-palette 0x00 0x3f 0x1f \
            --sprite-palette 0x00 \
            --panels 4x2,4 tiles/player.png \
            --background tiles/panel.png \
            --panels 6x3,2 tiles/castles.png \
            --panels 1x2,2 tiles/fence.png \
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
        echo "   -> ${file}.c"
        ${sdcc} -c -mz80 --peep-file ${devkitSMS}/SMSlib/src/peep-rules.txt -I ${SMSlib}/src \
            -o "build/code/${file}.rel" "source/${file}.c" || exit 1
    done

    # Libraries
    ${sdcc} -c -mz80 --peep-file ${devkitSMS}/SMSlib/src/peep-rules.txt -I ${SMSlib}/src \
        -o "build/code/fxsample.rel" "libraries/sms-fxsample/fxsample.c" || exit 1

    # Asset banks
    for bank in 2 3 4 5 6 7 8 9 10
    do
        ${sdcc} -c -mz80 --constseg BANK_${bank} source/bank_${bank}.c -o build/bank_${bank}.rel
    done

    echo ""
    echo "  Linking..."
    ${sdcc} -o build/Ants.ihx -mz80 --no-std-crt0 --data-loc 0xC000 \
        -Wl-b_BANK_2=0x8000 -Wl-b_BANK_3=0x8000 -Wl-b_BANK_4=0x8000 \
        -Wl-b_BANK_5=0x8000 -Wl-b_BANK_6=0x8000 -Wl-b_BANK_7=0x8000 \
        -Wl-b_BANK_8=0x8000 -Wl-b_BANK_9=0x8000 -Wl-b_BANK_10=0x8000 \
        ${devkitSMS}/crt0/crt0_sms.rel \
        build/code/*.rel \
        ${SMSlib}/SMSlib.lib \
        build/bank_2.rel build/bank_3.rel build/bank_4.rel \
        build/bank_5.rel build/bank_6.rel build/bank_7.rel \
        build/bank_8.rel build/bank_9.rel build/bank_10.rel || exit 1

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
