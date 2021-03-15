#include <stdint.h>

#include "midi.h"

#define MIDI_CHECKS

#define MIDI_NOTE_OFF            0x80
#define MIDI_NOTE_ON             0x90
#define MIDI_POLYPHONIC_PRESSURE 0xa0
#define MIDI_CONTROL_CHANGE      0xb0
#define MIDI_PROGRAM_CHANGE      0xc0
#define MIDI_CHANNEL_PRESSURE    0xd0
#define MIDI_PITCH_WHEEL_CHANGE  0xe0
#define MIDI_SYSTEM              0xf0

uint8_t *midi_note_off(uint8_t *buffer, uint8_t channel, uint8_t key, uint8_t velocity)
{
#ifdef MIDI_CHECKS
    channel &= 0xf;
    key &= 0x7f;
    velocity &= 0x7f;
#endif

    buffer[0] = MIDI_NOTE_OFF | channel;
    buffer[1] = key;
    buffer[2] = velocity;
    return buffer + 3;
}

uint8_t *midi_note_on(uint8_t *buffer, uint8_t channel, uint8_t key, uint8_t velocity)
{
#ifdef MIDI_CHECKS
    channel &= 0xf;
    key &= 0x7f;
    velocity &= 0x7f;
#endif

    buffer[0] = MIDI_NOTE_ON | channel;
    buffer[1] = key;
    buffer[2] = velocity;
    return buffer + 3;
}

uint8_t *midi_polyphonic_pressure(uint8_t *buffer, uint8_t channel, uint8_t key, uint8_t velocity)
{
#ifdef MIDI_CHECKS
    channel &= 0xf;
    key &= 0x7f;
    velocity &= 0x7f;
#endif

    buffer[0] = MIDI_POLYPHONIC_PRESSURE | channel;
    buffer[1] = key;
    buffer[2] = velocity;
    return buffer + 3;
}

uint8_t *midi_control_change(uint8_t *buffer, uint8_t channel, uint8_t controller, uint8_t value)
{
#ifdef MIDI_CHECKS
    channel &= 0xf;
    controller &= 0x7f;
    value &= 0x7f;
#endif

    buffer[0] = MIDI_CONTROL_CHANGE | channel;
    buffer[1] = controller;
    buffer[2] = value;
    return buffer + 3;
}

uint8_t *midi_program_change(uint8_t *buffer, uint8_t channel, uint8_t program)
{
#ifdef MIDI_CHECKS
    channel &= 0xf;
    program &= 0x7f;
#endif

    buffer[0] = MIDI_PROGRAM_CHANGE | channel;
    buffer[1] = program;
    return buffer + 2;
}

uint8_t *midi_channel_pressure(uint8_t *buffer, uint8_t channel, uint8_t value)
{
#ifdef MIDI_CHECKS
    channel &= 0xf;
    value &= 0x7f;
#endif

    buffer[0] = MIDI_CHANNEL_PRESSURE | channel;
    buffer[1] = value;
    return buffer + 2;
}

uint8_t *midi_pitch_wheel_change(uint8_t *buffer, uint8_t channel, uint16_t value)
{
#ifdef MIDI_CHECKS
    channel &= 0xf;
#endif

    uint8_t v1 = (value & 0x7f);
    uint8_t v2 = ((value >> 7) & 0x7f);

    buffer[0] = MIDI_PITCH_WHEEL_CHANGE | channel;
    buffer[1] = v1;
    buffer[2] = v2;
    return buffer + 3;
}

uint8_t *midi_system(uint8_t *buffer, enum midi_system_e command)
{
    *buffer = MIDI_SYSTEM | command;
    return buffer + 1;
}

uint8_t *midi_song_pointer(uint8_t *buffer, uint16_t pointer)
{
    uint8_t v1 = (pointer & 0x7f);
    uint8_t v2 = ((pointer >> 7) & 0x7f);

    buffer[0] = MIDI_SYSTEM | MIDI_SYS_SONG_POINTER;
    buffer[1] = v1;
    buffer[2] = v2;

    return buffer + 3;
}

uint8_t *midi_song_select(uint8_t *buffer, uint8_t song)
{
#ifdef MIDI_CHECKS
    song &= 0x7f;
#endif
    buffer[0] = MIDI_SYSTEM | MIDI_SYS_SONG_SELECT;
    buffer[1] = song;

    return buffer + 2;
}
