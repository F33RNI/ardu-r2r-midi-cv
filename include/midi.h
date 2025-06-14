/**
 * @file midi.h
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

#ifndef MIDI_H__
#define MIDI_H__

#include <Arduino.h>

#include <midiXparser.h>

// Serial begin and read functions
// NOTE: This may conflict with DEBUG_INIT in "include/calibration.h" file
#ifndef SERIAL_DEBUG
#define MIDI_SERIAL_INIT Serial.begin(31250UL)
#else
#define MIDI_SERIAL_INIT
#endif
#define MIDI_SERIAL_AVAILABLE Serial.available()
#define MIDI_SERIAL_READ      Serial.read()

// For middle point calculation in polyphonic mode (0-1, closer to 1, slower middle point will be moving)
#define MIDPOINT_FILTER_K .65f

// Ignore notes that are lower
#define NOTE_MIN 12U

struct notesEnabled {
    uint64_t msb, lsb;
};

class MIDI {
  public:
    void init(void);
    void loop(void);
    void set_note(uint8_t channel, uint8_t note, boolean state);
    boolean is_note_enabled(uint8_t channel, uint8_t note);
    uint8_t get_next_note(uint8_t channel, uint8_t note_last, boolean up);
    boolean get_channel_gate(uint8_t channel);
    boolean get_gate_from_midpoint(boolean to_right);
    boolean omni, note_1_event_on, note_2_event_on, note_1_event_off, note_2_event_off, pitch_bend_event;
    boolean panic_1_event, panic_2_event;
    uint8_t note_1, note_2, note_last;
    int16_t pitch_bend;
    float note_midpoint;

  private:
    midiXparser voice_parser, clock_parser;
    uint64_t clock_tick_last_time;
    struct notesEnabled notes_enabled_1, notes_enabled_2;
};

extern MIDI midi;

#endif
