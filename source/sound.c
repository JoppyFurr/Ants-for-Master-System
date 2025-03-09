/*
 * Ants for Master System
 * An Ants clone for the Sega Master System
 *
 * Sound playback
 */

#include <stdint.h>

#include "SMSlib.h"
#include "../libraries/sms-fxsample/fxsample.h"

extern const uint8_t card_sound [];
extern const uint8_t increase_power_sound [];
extern const uint8_t build_castle_sound [];
extern const uint8_t build_fence_sound [];
extern const uint8_t ruin_castle_sound [];
extern const uint8_t ruin_fence_sound [];
extern const uint8_t increase_stocks_sound [];
extern const uint8_t decrease_stocks_sound [];
extern const uint8_t curse_sound [];
extern const uint8_t fanfare_sound_1 [];
extern const uint8_t fanfare_sound_2 [];

/*
 * Play the card-slide sound.
 */
void play_card_sound (void)
{
    SMS_mapROMBank (3);
    PlaySample ((void *) card_sound);
    SMS_mapROMBank (2); /* By default, keep the VDP patterns mapped */
}


/*
 * Play the increase-power sound.
 */
void play_increase_power_sound (void)
{
    SMS_mapROMBank (3);
    PlaySample ((void *) increase_power_sound);
    SMS_mapROMBank (2); /* By default, keep the VDP patterns mapped */
}


/*
 * Play the build-castle sound.
 */
void play_build_castle_sound (void)
{
    SMS_mapROMBank (4);
    PlaySample ((void *) build_castle_sound);
    SMS_mapROMBank (2); /* By default, keep the VDP patterns mapped */
}


/*
 * Play the build-fence sound.
 */
void play_build_fence_sound (void)
{
    SMS_mapROMBank (4);
    PlaySample ((void *) build_fence_sound);
    SMS_mapROMBank (2); /* By default, keep the VDP patterns mapped */
}


/*
 * Play the ruin-castle sound.
 */
void play_ruin_castle_sound (void)
{
    SMS_mapROMBank (5);
    PlaySample ((void *) ruin_castle_sound);
    SMS_mapROMBank (2); /* By default, keep the VDP patterns mapped */
}


/*
 * Play the ruin-fence sound.
 */
void play_ruin_fence_sound (void)
{
    SMS_mapROMBank (5);
    PlaySample ((void *) ruin_fence_sound);
    SMS_mapROMBank (2); /* By default, keep the VDP patterns mapped */
}


/*
 * Play the increase-stocks sound.
 */
void play_increase_stocks_sound (void)
{
    SMS_mapROMBank (6);
    PlaySample ((void *) increase_stocks_sound);
    SMS_mapROMBank (2); /* By default, keep the VDP patterns mapped */
}


/*
 * Play the decrease-stocks sound.
 */
void play_decrease_stocks_sound (void)
{
    SMS_mapROMBank (6);
    PlaySample ((void *) decrease_stocks_sound);
    SMS_mapROMBank (2); /* By default, keep the VDP patterns mapped */
}


/*
 * Play the decrease-stocks sound.
 */
void play_curse_sound (void)
{
    SMS_mapROMBank (7);
    PlaySample ((void *) curse_sound);
    SMS_mapROMBank (2); /* By default, keep the VDP patterns mapped */
}


/*
 * Play the fanfare sound.
 */
void play_fanfare_sound (void)
{
    SMS_mapROMBank (8);
    PlaySample ((void *) fanfare_sound_1);
    SMS_mapROMBank (9);
    PlaySample ((void *) fanfare_sound_2);
    SMS_mapROMBank (2); /* By default, keep the VDP patterns mapped */
}
