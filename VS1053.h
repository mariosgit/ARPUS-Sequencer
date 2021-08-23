#pragma once

#include <Arduino.h>

struct VS1053
{
    static void setup();
    static void loop();
    static void midiSetInstrument(uint8_t chan, uint8_t inst);
    static void midiSetChannelVolume(uint8_t chan, uint8_t vol);
    static void midiSetChannelBank(uint8_t chan, uint8_t bank);
    static void midiNoteOn(uint8_t chan, uint8_t n, uint8_t vel);
    static void midiNoteOff(uint8_t chan, uint8_t n, uint8_t vel);
};

