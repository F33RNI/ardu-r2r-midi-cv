/**
 * @file midi.cpp
 * @author Fern Lane
 * @brief Main MIDI handler
 * Uses <https://github.com/TheKikGen/midiXparser> library
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

#include "include/midi.h"
#include "include/clock.h"
#include "include/pins.h"

// Preinstantiate
MIDI midi;

/**
 * @brief Initialises serial port and midiXparser library
 */
void MIDI::init(void) {
    MIDI_SERIAL_INIT;
    voice_parser.setMidiMsgFilter(midiXparser::channelVoiceMsgTypeMsk);
    clock_parser.setMidiMsgFilter(midiXparser::realTimeMsgTypeMsk);
}

/**
 * @brief Parses MIDI note ON/OFF and pitch bend events.
 * NOTE: You MUST handle `note_1_event_on` - `note_2_event_off` right after calling this
 */
void MIDI::loop(void) {
    if (!MIDI_SERIAL_AVAILABLE)
        return;

    uint8_t data = MIDI_SERIAL_READ;

    // Channel Voice Messages
    if (voice_parser.parse(data) && voice_parser.getMidiMsgLen() == 3U) {
        uint8_t channel = voice_parser.getMidiMsg()[0] & 0x0F;

        // Ignore events for other channels outside omni mode
        if (!omni && channel > 1U)
            return;

        // Note ON/OFF
        if (voice_parser.isMidiStatus(midiXparser::noteOnStatus) ||
            voice_parser.isMidiStatus(midiXparser::noteOffStatus)) {
            uint8_t note = voice_parser.getMidiMsg()[1] & 0x7F;
            if (note < NOTE_MIN)
                return;
            boolean on = voice_parser.isMidiStatus(midiXparser::noteOnStatus);

            // Ignore OFF events for notes that are already off
            if (!on && !is_note_enabled(channel, note))
                return;

            boolean &note_1_event = (on ? note_1_event_on : note_1_event_off);
            boolean &note_2_event = (on ? note_2_event_on : note_2_event_off);

            // Save event
            if (omni) {
                set_note(0U, note, on);
                set_note(1U, note, on);
                note_1_event = true;
                note_2_event = true;
            } else {
                set_note(channel, note, on);
                (channel ? note_2_event : note_1_event) = true;
            }

            // Save note number
            // if (on) {
            if (omni) {
                note_1 = note;
                note_2 = note;
            } else {
                (channel ? note_2 : note_1) = note;
            }

            if (note_midpoint == 0.f)
                note_midpoint = (static_cast<float>(note) + static_cast<float>(note_last)) / 2.f;
            else {
                note_midpoint = MIDPOINT_FILTER_K * note_midpoint;
                note_midpoint +=
                    ((static_cast<float>(note) + static_cast<float>(note_last)) / 2.f) * (1.f - MIDPOINT_FILTER_K);
            }

            note_last = note;
            //}

        }

        // Pitch bend
        else if (voice_parser.isMidiStatus(midiXparser::pitchBendStatus)) {
            // +/- 200 cents (+/- 2 semitones)
            pitch_bend = static_cast<int16_t>(voice_parser.getMidiMsg()[2] & 0x7F) << 7U;
            pitch_bend |= static_cast<int16_t>(voice_parser.getMidiMsg()[1] & 0x7F);
            pitch_bend = (pitch_bend - 8192) / 41;
            pitch_bend_event = true;
        }

        // All notes off event
        else if (voice_parser.isMidiStatus(midiXparser::controlChangeStatus) &&
                 (voice_parser.getMidiMsg()[1] == 120U || voice_parser.getMidiMsg()[1] == 123U) &&
                 voice_parser.getMidiMsg()[2] == 0U) {
            if (omni) {
                notes_enabled_1.msb = 0ULL;
                notes_enabled_1.lsb = 0ULL;
                notes_enabled_2.msb = 0ULL;
                notes_enabled_2.lsb = 0ULL;
                note_1_event_off = true;
                note_2_event_off = true;
                panic_1_event = true;
                panic_2_event = true;
            } else {
                struct notesEnabled *notes_enabled = (channel ? &notes_enabled_2 : &notes_enabled_1);
                notes_enabled->msb = 0ULL;
                notes_enabled->lsb = 0ULL;
                (channel ? note_2_event_off : note_1_event_off) = true;
                (channel ? panic_2_event : panic_1_event) = true;
            }
        }
    }

    // Clock pulse
    if (clock_parser.parse(data) && clock_parser.isMidiStatus(midiXparser::timingClockStatus))
        clock.midi_tick();
}

/**
 * @brief Saves note state into `notes_enabled_1` / `notes_enabled_2`
 *
 * @param channel 0 to use `notes_enabled_1`, 1 to use `notes_enabled_2`
 * @param note 0-127
 * @param state true if note is ON, false if note is OFF
 */
void MIDI::set_note(uint8_t channel, uint8_t note, boolean state) {
    struct notesEnabled *notes_enabled = (channel ? &notes_enabled_2 : &notes_enabled_1);
    uint64_t note_mask = 1ULL << (note % 64U);
    if (state)
        (note > 63U ? notes_enabled->msb : notes_enabled->lsb) |= note_mask;
    else
        (note > 63U ? notes_enabled->msb : notes_enabled->lsb) &= ~note_mask;
}

/**
 * @brief Checks note state in `notes_enabled_1` / `notes_enabled_2`
 *
 * @param channel 0 to use `notes_enabled_1`, 1 to use `notes_enabled_2`
 * @param note 0-127
 * @return boolean true if note is ON, false if note is OFF
 */
boolean MIDI::is_note_enabled(uint8_t channel, uint8_t note) {
    struct notesEnabled *notes_enabled = (channel ? &notes_enabled_2 : &notes_enabled_1);
    uint64_t note_mask = 1ULL << (note % 64U);
    return (note > 63U ? notes_enabled->msb : notes_enabled->lsb) & note_mask;
}

/**
 * @brief Cycles though all notes in `notes_enabled_1` / `notes_enabled_2` starting from note_last and
 * tries to find next note (for arpeggiator)
 *
 * @param channel 0 to use `notes_enabled_1`, 1 to use `notes_enabled_2`
 * @param note_last starting point (will NOT be included)
 * @param up direction
 * @return uint8_t 0-127 (next note or same one) or 255 if ALL notes are off
 */
uint8_t MIDI::get_next_note(uint8_t channel, uint8_t note_last, boolean up) {
    struct notesEnabled *notes_enabled = (channel ? &notes_enabled_2 : &notes_enabled_1);
    uint8_t note = note_last;
    for (;;) {
        // Calculate next note number
        note = up ? (note + 1) % 128 : (note == 0 ? 127 : note - 1);

        // Return same note if it's still ON or 255 in case of no ON notes
        if (note == note_last)
            return ((note > 63U ? notes_enabled->msb : notes_enabled->lsb) & (1ULL << (note % 64U))) ? note : 255U;

        // Next note found
        if ((note > 63U ? notes_enabled->msb : notes_enabled->lsb) & (1ULL << (note % 64U)))
            return note;
    }
}

/**
 * @brief Check if at least 1 note is ON
 *
 * @param channel 0-1
 * @return boolean true if at least 1 note is ON
 */
boolean MIDI::get_channel_gate(uint8_t channel) {
    struct notesEnabled *notes_enabled = (channel ? &notes_enabled_2 : &notes_enabled_1);
    return ((notes_enabled->msb || notes_enabled->lsb) ? true : false);
}

/**
 * @brief Check if at least 1 note is ON on the right or on the left of note_midpoint (INCLUDING midpoint itself)
 *
 * @param to_right true to check all notes from midpoint to the right (0->127)
 * @return boolean true if at least 1 note is ON
 */
boolean MIDI::get_gate_from_midpoint(boolean to_right) {
    uint8_t start;
    if (fmodf(note_midpoint, 1.f) == 0.f)
        start = static_cast<uint8_t>(note_midpoint);
    else {
        start = static_cast<uint8_t>(note_midpoint);
        if (to_right)
            start++;
    }

    for (uint8_t note = start; (to_right ? note < 127U : note > 0U); (to_right ? ++note : --note))
        if ((note > 63U ? notes_enabled_1.msb : notes_enabled_1.lsb) & (1ULL << (note % 64U)))
            return true;
    return false;
}
