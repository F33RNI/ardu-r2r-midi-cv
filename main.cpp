/**
 * @file main.cpp
 * @author Fern Lane
 * @brief Main file that contains setup() and loop()
 *
 * @copyright Copyright (c) 2022-2025 Fern Lane
 *
 * This file is part of the ardu-r2r-midi-cv (aka CMCEC) distribution.
 * See <https://github.com/F33RNI/ardu-r2r-midi-cv> for more info.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Arduino.h>

#include "include/calibration.h"
#include "include/clock.h"
#include "include/dac.h"
#include "include/dip_switch.h"
#include "include/gate_trig.h"
#include "include/leds.h"
#include "include/midi.h"

// Default notes at startup in cents (6000 cents = note 60 = C4 (aka middle C))
#define NOTE_START_1_CENTS 6000
#define NOTE_START_2_CENTS 6000

int16_t target_cents_1, target_cents_2;
uint8_t arp_note_1, arp_note_2;

// Methods declaration (see bottom of this file)
void direct_channel_mode(uint8_t channel), split_channel_mode(void), arp_mode(uint8_t channel, boolean up);
void write_to_channel(boolean channel_1, boolean channel_2);

void setup() {
    leds.init();
    dac.init();
    gate_trig.init();
    dip_switch.init();
    calibration.init_check();
    if (!calibration.active) {
        midi.init();
        clock.init();
        clock.set_source(ClockSource::EXT);
        target_cents_1 = NOTE_START_1_CENTS;
        target_cents_2 = NOTE_START_2_CENTS;
        write_to_channel(true, true);
    }
}

void loop() {
    dip_switch.read();

    calibration.loop();

    if (!calibration.active) {
        midi.loop();
        clock.loop();

        // Prase DIP switch (see manual for more info)
        midi.omni = dip_switch.states & 0x80U;
        gate_trig.merged = dip_switch.states & 0b01000000U;
        clock.divider = (dip_switch.states & 0b00110000U) >> 4U;
        boolean arp_1_enabled = dip_switch.states & 0b00001000U;
        boolean arp_2_enabled = dip_switch.states & 0b00000100U;
        boolean arp_1_up = dip_switch.states & 0b00000010U;
        boolean arp_2_up = dip_switch.states & 0b00000001U;
        boolean split_left_right = !(dip_switch.states & 0b00001100U) && (dip_switch.states & 0b00000011U);

        // Reset arpeggiators
        if (!arp_1_enabled)
            arp_note_1 = 255U;
        if (!arp_2_enabled)
            arp_note_2 = 255U;

        // Set gates OFF on panic event
        if (midi.panic_1_event) {
            gate_trig.set_1(false);
            midi.panic_1_event = false;
        }
        if (midi.panic_2_event) {
            gate_trig.set_2(false);
            midi.panic_2_event = false;
        }

        if (arp_1_enabled)
            arp_mode(0U, arp_1_up);
        else if (!split_left_right)
            direct_channel_mode(0U);

        if (arp_2_enabled)
            arp_mode(1U, arp_2_up);
        else if (!split_left_right)
            direct_channel_mode(1U);

        if (split_left_right)
            split_channel_mode();

        // Handle pitch bend
        if (midi.pitch_bend_event) {
            midi.pitch_bend_event = false;
            write_to_channel(true, true);
        }
    }

    // Write everything
    gate_trig.loop();
    leds.loop();
    dac.calculate_compensation();
    dac.write();

    // Clear event only after both arpeggiator and LEDs
    clock.clock_event = false;
}

/**
 * @brief Simplest mode. Just writes note from MIDI into CV and starts / stop gate and trigger
 *
 * @param channel 0 or 1
 */
void direct_channel_mode(uint8_t channel) {
    bool &event_on = (channel ? midi.note_2_event_on : midi.note_1_event_on);
    bool &event_off = (channel ? midi.note_2_event_off : midi.note_1_event_off);
    if (!event_on && !event_off)
        return;

    // Write CV
    if (event_on) {
        (channel ? target_cents_2 : target_cents_1) = static_cast<int16_t>(channel ? midi.note_2 : midi.note_1) * 100;
        if (channel)
            write_to_channel(false, true);
        else
            write_to_channel(true, false);
    }

    // All notes on channel are OFF
    if (event_off && !midi.get_channel_gate(channel)) {
        if (channel)
            gate_trig.set_2(false);
        else
            gate_trig.set_1(false);
    }

    // Start gate / retrigger
    else if (event_on) {
        if (channel)
            gate_trig.set_2(true);
        else
            gate_trig.set_1(true);
    }

    // Clear events (because we handled them)
    event_on = false;
    event_off = false;
    midi.pitch_bend_event = false;
}

/**
 * @brief Sends 2 notes on 1 channel into different ports based on their location to each other
 * (higher note will be sent to port 2, while lowest to the 1st). Without omni mode enabled, only 1st MIDI channel will
 * be used.
 */
void split_channel_mode(void) {
    if (!midi.note_1_event_on && !midi.note_1_event_off)
        return;

    boolean channel = static_cast<float>(midi.note_1) > midi.note_midpoint;
    (channel ? target_cents_2 : target_cents_1) = static_cast<int16_t>(midi.note_1) * 100;

    // Start gate / retrigger
    if (midi.note_1_event_on) {
        if (channel) {
            write_to_channel(false, true);
            gate_trig.set_2(true);
        } else {
            write_to_channel(true, false);
            gate_trig.set_1(true);
        }
    }

    // Check gate on both sides and turn it off if no more notes
    else if (midi.note_1_event_off && !midi.get_gate_from_midpoint(channel)) {
        if (channel)
            gate_trig.set_2(false);
        else
            gate_trig.set_1(false);
    }

    // Clear events (because we handled them)
    midi.note_1_event_on = false;
    midi.note_1_event_off = false;
    midi.note_2_event_on = false;
    midi.note_2_event_off = false;
    midi.pitch_bend_event = false;
}

/**
 * @brief Arpeggiator mode (clock (MIDI or external) must be present for it to work).
 * NOTE: `clock.clock_event` must be cleared outside this function
 *
 * @param channel 0 / 2
 * @param up direction
 */
void arp_mode(uint8_t channel, boolean up) {
    // Clear events (because we don't care in this mode)
    midi.note_1_event_on = false;
    midi.note_1_event_off = false;
    midi.note_2_event_on = false;
    midi.note_2_event_off = false;

    // Wait for clock
    if (!clock.clock_event)
        return;

    // Calculate starting point
    uint8_t &arp_note = (channel ? arp_note_2 : arp_note_1);
    if (arp_note > 127U)
        arp_note = (up ? 0U : 127U);

    // Look for next note
    arp_note = midi.get_next_note(channel, arp_note, up);

    // No more notes -> set both gates to OFF
    if (arp_note > 127U) {
        if (channel)
            gate_trig.set_2(false);
        else
            gate_trig.set_1(false);
        return;
    }

    // Write note and set gate ON / retrigger
    (channel ? target_cents_2 : target_cents_1) = static_cast<int16_t>(arp_note) * 100;
    if (channel) {
        write_to_channel(false, true);
        gate_trig.set_2(true);
    } else {
        write_to_channel(true, false);
        gate_trig.set_1(true);
    }
}

/**
 * @brief Writes `target_cents_1` / `target_cents_2` + `midi.pitch_bend` to the DAC and LEDs
 *
 * @param channel_1 true to write to 1st channel
 * @param channel_2 true to write to 2nd channel
 */
void write_to_channel(boolean channel_1, boolean channel_2) {
    leds.cents_1 = target_cents_1;
    leds.cents_2 = target_cents_2;
    leds.pitch_bend = midi.pitch_bend;
    if (channel_1 && channel_2) {
        float mv_1 = calibration.note_to_mv_cal(0U, static_cast<uint16_t>(target_cents_1 + midi.pitch_bend));
        float mv_2 = calibration.note_to_mv_cal(1U, static_cast<uint16_t>(target_cents_2 + midi.pitch_bend));
        dac.set(mv_1, mv_2);
    } else if (channel_1) {
        float mv = calibration.note_to_mv_cal(0U, static_cast<uint16_t>(target_cents_1 + midi.pitch_bend));
        dac.set(mv, NAN);
    } else {
        float mv = calibration.note_to_mv_cal(1U, static_cast<uint16_t>(target_cents_2 + midi.pitch_bend));
        dac.set(NAN, mv);
    }
}
