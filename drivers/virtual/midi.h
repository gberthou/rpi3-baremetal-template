#ifndef DRIVERS_MIDI_H
#define DRIVERS_MIDI_H

#include <stdint.h>

// http://www.music.mcgill.ca/~ich/classes/mumt306/StandardMIDIfileformat.html

enum midi_note_e
{
    MIDI_NOTE_C  = 0,
    MIDI_NOTE_CS,
    MIDI_NOTE_D,
    MIDI_NOTE_DS,
    MIDI_NOTE_E,
    MIDI_NOTE_F,
    MIDI_NOTE_FS,
    MIDI_NOTE_G,
    MIDI_NOTE_GS,
    MIDI_NOTE_A,
    MIDI_NOTE_AS,
    MIDI_NOTE_B,
};

enum midi_system_e
{
    MIDI_SYS_SONG_POINTER   = 0x2,
    MIDI_SYS_SONG_SELECT    = 0x3,
    MIDI_SYS_TUNE_REQUEST   = 0x6,
    MIDI_SYS_TIMING_CLOCK   = 0x8,
    MIDI_SYS_START          = 0xa,
    MIDI_SYS_CONTINUE       = 0xb,
    MIDI_SYS_STOP           = 0xc,
    MIDI_SYS_ACTIVE_SENSING = 0xe,
    MIDI_SYS_RESET          = 0xf
};

enum midi_control_e
{
    MIDI_CTRL_BANK_SELECT_MSB = 0x00,
    MIDI_CTRL_MODULATION_MSB,
    MIDI_CTRL_BREATH_MSB,
    MIDI_CTRL_FOOT_MSB = 0x03,
    MIDI_CTRL_PORTAMENTO_TIME_MSB,
    MIDI_CTRL_DATA_ENTRY_MSB,
    MIDI_CTRL_CHAN_VOLUME_MSB,
    MIDI_CTRL_BALANCE_MSB,
    MIDI_CTRL_PAN_MSB = 0x0a,
    MIDI_CTRL_EXPRESSION_MSB,
    MIDI_CTRL_EFFECT1_MSB,
    MIDI_CTRL_EFFECT2_MSB,
    MIDI_CTRL_GP1_MSB = 0x10,
    MIDI_CTRL_GP2_MSB,
    MIDI_CTRL_GP3_MSB,
    MIDI_CTRL_GP4_MSB,

    MIDI_CTRL_BANK_SELECT_LSB = 0x20,
    MIDI_CTRL_MODULATION_LSB,
    MIDI_CTRL_BREATH_LSB,
    MIDI_CTRL_FOOT_LSB = 0x23,
    MIDI_CTRL_PORTAMENTO_TIME_LSB,
    MIDI_CTRL_DATA_ENTRY_LSB,
    MIDI_CTRL_CHAN_VOLUME_LSB,
    MIDI_CTRL_BALANCE_LSB,
    MIDI_CTRL_PAN_LSB = 0x2a,
    MIDI_CTRL_EXPRESSION_LSB,
    MIDI_CTRL_EFFECT1_LSB,
    MIDI_CTRL_EFFECT2_LSB,
    MIDI_CTRL_GP1_LSB = 0x30,
    MIDI_CTRL_GP2_LSB,
    MIDI_CTRL_GP3_LSB,
    MIDI_CTRL_GP4_LSB,

    MIDI_CTRL_DAMPER = 0x40,
    MIDI_CTRL_PORTAMENTO_ONOFF,
    MIDI_CTRL_SUSTENUTO_ONOFF,
    MIDI_CTRL_SOFT_ONOFF,
    MIDI_CTRL_LEGATO_ONOFF,
    MIDI_CTRL_HOLD2_ONOFF,
    MIDI_CTRL_SND1,
    MIDI_CTRL_SND2,
    MIDI_CTRL_SND3,
    MIDI_CTRL_SND4,
    MIDI_CTRL_SND5,
    MIDI_CTRL_SND6,
    MIDI_CTRL_SND7,
    MIDI_CTRL_SND8,
    MIDI_CTRL_SND9,
    MIDI_CTRL_SND10,
    MIDI_CTRL_GP5,
    MIDI_CTRL_GP6,
    MIDI_CTRL_GP7,
    MIDI_CTRL_GP8,
    MIDI_CTRL_PORTAMENTO,
    MIDI_CTRL_EFFECT1_DEPTH = 0x5b,
    MIDI_CTRL_EFFECT2_DEPTH,
    MIDI_CTRL_EFFECT3_DEPTH,
    MIDI_CTRL_EFFECT4_DEPTH,
    MIDI_CTRL_EFFECT5_DEPTH,
    MIDI_CTRL_DATA_PLUS1,
    MIDI_CTRL_DATA_MINUS1,

    MIDI_CTRL_SND_OFF = 0x78,
    MIDI_CTRL_RESET,
    MIDI_CTRL_ONOFF,
    MIDI_CTRL_NOTES_OFF,
    MIDI_CTRL_OMNI_OFF,
    MIDI_CTRL_OMNI_ON,
    MIDI_CTRL_POLY_OFF,
    MIDI_CTRL_POLY_ON
};

// note: enum midi_note_e
// -1 <= octave <= 9
#define MIDI_NOTE_TO_KEY(note, octave) (12 * ((octave) + 1) + (note))

uint8_t *midi_note_off(uint8_t *buffer, uint8_t channel, uint8_t key, uint8_t velocity);
uint8_t *midi_note_on(uint8_t *buffer, uint8_t channel, uint8_t key, uint8_t velocity);
uint8_t *midi_polyphonic_pressure(uint8_t *buffer, uint8_t channel, uint8_t key, uint8_t velocity);
uint8_t *midi_control_change(uint8_t *buffer, uint8_t channel, uint8_t controller, uint8_t value);
uint8_t *midi_program_change(uint8_t *buffer, uint8_t channel, uint8_t program);
uint8_t *midi_channel_pressure(uint8_t *buffer, uint8_t channel, uint8_t value);
uint8_t *midi_pitch_wheel_change(uint8_t *buffer, uint8_t channel, uint16_t value);
uint8_t *midi_system(uint8_t *buffer, enum midi_system_e command);
uint8_t *midi_song_pointer(uint8_t *buffer, uint16_t pointer);
uint8_t *midi_song_select(uint8_t *buffer, uint8_t song);

#endif

