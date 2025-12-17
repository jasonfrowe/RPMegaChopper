#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

/**
 * sound.h - PSG (Programmable Sound Generator) sound system
 * 
 * Manages 8-channel sound effects with round-robin allocation
 */

// Waveform types
typedef enum {
    PSG_WAVE_SINE = 0,
    PSG_WAVE_SQUARE = 1,
    PSG_WAVE_SAWTOOTH = 2,
    PSG_WAVE_TRIANGLE = 3,
    PSG_WAVE_NOISE = 4
} PSGWaveform;

// Sound effect types (for round-robin channel allocation)
typedef enum {
    SFX_TYPE_PLAYER_FIRE = 0,
    SFX_TYPE_ENEMY_FIRE = 1,
    SFX_TYPE_EXPLOSION = 2,     // Channels 4, 5 (New)
    SFX_TYPE_EVENT = 3,         // Channels 6, 7 (New)
    SFX_TYPE_COUNT = 4
} SFXType;

/**
 * Initialize the PSG sound system
 */
extern void init_psg(void);

/**
 * Play a sound effect with round-robin channel allocation
 * @param sfx_type Sound effect type (determines channel allocation)
 * @param freq Frequency in Hz
 * @param wave Waveform type
 * @param attack Attack rate (0-15)
 * @param decay Decay rate (0-15)
 * @param release Release rate (0-15)
 * @param volume Volume (0-15, where 0=loud, 15=silent)
 */
extern void play_sound(uint8_t sfx_type, uint16_t freq, uint8_t wave, 
                uint8_t attack, uint8_t decay, uint8_t release, uint8_t volume);
extern void sfx_player_shoot(void);
extern void sfx_enemy_shoot(void);

// Play a small explosion (bullet hitting ground/wall)
extern void sfx_explosion_small(void);

// Play a large explosion (Tank death, Chopper crash)
extern void sfx_explosion_large(void);

// Play a "Whistle" for a falling bomb
extern void sfx_bomb_drop(void);

// Play a "Ding" for rescuing a hostage
extern void sfx_hostage_rescue(void);

// Play a sad sound for hostage death
extern void sfx_hostage_die(void);

// Play chopper movement sound
extern void sfx_chopper(void);

extern void update_chopper_sound(uint16_t velocity_mag);

extern void stop_chopper_sound(void);

#endif // SOUND_H
