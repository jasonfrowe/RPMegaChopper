#include "sound.h"
#include "constants.h"
#include <rp6502.h>
#include <stdint.h>

// ============================================================================
// MODULE STATE
// ============================================================================

// Channel allocation (2 channels per effect type for round-robin)
static uint8_t next_channel[SFX_TYPE_COUNT] = {0, 2};

// ============================================================================
// INTERNAL FUNCTIONS
// ============================================================================

/**
 * Stop sound on a channel
 */
static void stop_sound(uint8_t channel)
{
    if (channel > 7) return;
    
    uint16_t psg_addr = PSG_XRAM_ADDR + (channel * 8) + 6;  // pan_gate offset
    RIA.addr0 = psg_addr;
    RIA.rw0 = 0x00;  // Gate off (release)
}

// ============================================================================
// PUBLIC FUNCTIONS
// ============================================================================

void init_psg(void)
{
    // Enable PSG at XRAM address PSG_XRAM_ADDR
    xregn(0, 1, 0x00, 1, PSG_XRAM_ADDR);
    
    // Clear all 8 channels (64 bytes total)
    RIA.addr0 = PSG_XRAM_ADDR;
    RIA.step0 = 1;
    for (uint8_t i = 0; i < 64; i++) {
        RIA.rw0 = 0;
    }
}

void play_sound(uint8_t sfx_type, uint16_t freq, uint8_t wave, 
                uint8_t attack, uint8_t decay, uint8_t release, uint8_t volume)
{
    if (sfx_type >= SFX_TYPE_COUNT) return;
    
    // Get base channel for this effect type (each type has 2 channels)
    uint8_t base_channel = sfx_type * 2;
    uint8_t channel = base_channel + next_channel[sfx_type];
    
    // Release the previous sound on the old channel
    uint8_t old_channel = base_channel + (1 - next_channel[sfx_type]);
    stop_sound(old_channel);
    
    // Toggle to next channel for this effect type
    next_channel[sfx_type] = 1 - next_channel[sfx_type];
    
    uint16_t psg_addr = PSG_XRAM_ADDR + (channel * 8);
    
    // Set frequency (Hz * 3)
    uint16_t freq_val = freq * 3;
    RIA.addr0 = psg_addr;
    RIA.rw0 = freq_val & 0xFF;          // freq low byte
    RIA.rw0 = (freq_val >> 8) & 0xFF;   // freq high byte
    
    // Set duty cycle (50%)
    RIA.rw0 = 128;
    
    // Set volume and attack
    RIA.rw0 = (volume << 4) | (attack & 0x0F);
    
    // Set decay volume to 15 (silent) so sound fades naturally without sustain
    RIA.rw0 = (15 << 4) | (decay & 0x0F);
    
    // Set waveform and release
    RIA.rw0 = (wave << 4) | (release & 0x0F);
    
    // Set pan (center) and gate (on)
    RIA.rw0 = 0x01;  // Center pan, gate on
}

// ============================================================================
// GAMEPLAY SOUND PRESETS
// ============================================================================

void sfx_player_shoot(void) {
    // Fast decay noise/square mix usually works well, but using Noise here
    // Freq 400Hz, Noise, Attack 0 (Instant), Decay 6 (Fast), Release 0, Vol 2
    // play_sound(SFX_TYPE_PLAYER_FIRE, 400, PSG_WAVE_NOISE, 0, 6, 0, 2);
    play_sound(SFX_TYPE_PLAYER_FIRE, 210, PSG_WAVE_SQUARE, 0, 3, 4, 2);
}

void sfx_enemy_shoot(void) {
    // Lower pitch, slightly longer
    // play_sound(SFX_TYPE_ENEMY_FIRE, 200, PSG_WAVE_SQUARE, 0, 8, 4, 2);
    play_sound(SFX_TYPE_ENEMY_FIRE, 440, PSG_WAVE_TRIANGLE, 0, 4, 3, 3);
}
void sfx_explosion_small(void) {
    // Quick pop. Low freq noise.
    // Freq 100, Noise, A:0, D:8, R:4, Vol: 2
    play_sound(SFX_TYPE_EXPLOSION, 100, PSG_WAVE_NOISE, 0, 6, 4, 2);
}

void sfx_explosion_large(void) {
    // Deep, long rumble. 
    // Freq 50, Noise, A:0, D:9 (Slow), R:10, Vol: 0 (Max Loud)
    play_sound(SFX_TYPE_EXPLOSION, 50, PSG_WAVE_NOISE, 0, 9, 10, 0);
}

void sfx_bomb_drop(void) {
    // High pitch "whistle" before impact.
    // Sawtooth gives a mechanical sound.
    // Freq 2000, Saw, A:4 (Slide in), D:10, R:4, Vol: 4
    play_sound(SFX_TYPE_EVENT, 1000, PSG_WAVE_SAWTOOTH, 4, 10, 4, 4);
}

void sfx_hostage_rescue(void) {
    // Positive "Ding".
    // Freq 1500, Sine/Square, A:0, D:8, R:8, Vol: 4
    play_sound(SFX_TYPE_EVENT, 1500, PSG_WAVE_SQUARE, 0, 8, 8, 4);
}

void sfx_hostage_die(void) {
    // Sad, low square wave.
    // Freq 150, Square, A:2, D:10, R:10, Vol: 4
    play_sound(SFX_TYPE_EVENT, 150, PSG_WAVE_SQUARE, 2, 10, 10, 4);
}

// --- ENGINE STATE ---
static uint8_t rotor_phase = 0;
static uint16_t current_engine_vol = 0;

#define ENG_CHAN        7
#define ENG_BASE_FREQ   80  // Low rumble
#define ENG_BASE_VOL    8   // 0 is Loud, 15 is Silent. 8 is mid-volume.

void update_chopper_sound(uint16_t velocity_mag) {
    uint16_t psg_addr = PSG_XRAM_ADDR + (ENG_CHAN * 8);
    
    // 1. Calculate Rotor Speed based on movement
    // Base speed + fraction of velocity
    // Velocity is subpixels (e.g., 0 to 48). 
    uint8_t rotor_speed = 1 + (velocity_mag >> 3); 
    
    // 2. Advance LFO (Low Frequency Oscillator)
    rotor_phase += rotor_speed;
    
    // 3. Calculate Volume Modulation (The "Wop-Wop")
    // We want the volume to dip periodically.
    // Triangle wave shape derived from phase:
    // 0..127 -> Loud to Quiet, 128..255 -> Quiet to Loud
    int8_t mod = (rotor_phase & 0x80) ? (0xFF - rotor_phase) : rotor_phase; // 0..127
    mod = mod >> 4; // Scale to 0..7
    
    // Final Volume: Base (8) minus modulation (closer to 0 is louder)
    // Range: 8 (Quiet) to 1 (Loud)
    uint8_t final_vol = ENG_BASE_VOL - mod; 
    if (final_vol > 15) final_vol = 15; // Safety cap (Silent)

    // 4. Calculate Pitch (Doppler-ish effect)
    // Higher speed = Higher pitch
    uint16_t freq = ENG_BASE_FREQ + (velocity_mag >> 2);
    
    // --- WRITE TO HARDWARE ---
    
    // Set Frequency
    uint16_t freq_val = freq * 3; // PSG scaling
    RIA.addr0 = psg_addr;
    RIA.rw0 = freq_val & 0xFF;
    RIA.rw0 = (freq_val >> 8) & 0xFF;
    
    // Duty Cycle (Noise/Square)
    RIA.rw0 = 128;
    
    // Volume / Attack
    // Vol = final_vol, Attack = 0 (Instant changes for LFO)
    RIA.rw0 = (final_vol << 4) | 0;
    
    // Decay / Sustain
    // We want infinite sustain for the beat. 
    // Set Decay Target to Same as Volume, Rate 0
    RIA.rw0 = (final_vol << 4) | 0;
    
    // Waveform / Release
    // Wave 4 (Noise) gives a "windy" chopper sound. 
    // Wave 1 (Square) gives a "buzzy" drone.
    // Try NOISE (4) first.
    RIA.rw0 = (PSG_WAVE_TRIANGLE << 4) | 0;
    
    // Gate On
    RIA.rw0 = 0x01;
}

void stop_chopper_sound(void) {
    stop_sound(ENG_CHAN);
}