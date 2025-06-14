/**
 * @file clock.h
 * @author Fern Lane
 * @brief Clock port handler
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

#ifndef CLOCK_H__
#define CLOCK_H__

#include <Arduino.h>

// Duration of clock pulse (in milliseconds)
#define CLOCK_HIGH_DURATION 10U

// Uncomment to invert clock (pulse will be LOW)
// #define CLOCK_INVERTED

// Switch clock source to external if no MIDI clock present in this time (milliseconds)
#define MIDI_CLOCK_TIMEOUT 2000

enum class ClockSource : uint8_t { NONE, MIDI, EXT };

class Clock {
  public:
    void init(void);
    void set_source(enum ClockSource source);
    void midi_tick(void);
    void loop(void);
    enum ClockSource source;
    uint8_t divider;
    boolean clock_event;

  private:
    volatile uint8_t *port_out_reg;
    uint8_t pin_mask;
    uint64_t midi_tick_time_last, on_time;
    uint8_t ticks_counter;
    volatile uint8_t ticks_counter_ext;
    volatile boolean clock_event_ext;

    void write_output(boolean state);
    void handle_interrupt(void);
    static void isr(void);
};

extern Clock clock;

#endif
