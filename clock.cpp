/**
 * @file clock.cpp
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

#include "include/clock.h"
#include "include/pins.h"

#include <util/atomic.h>

// Preinstantiate
Clock clock;

/**
 * @brief Just saves clock's digital port for further enabling
 */
void Clock::init(void) {
    port_out_reg = portOutputRegister(digitalPinToPort(PIN_CLOCK));
    pin_mask = digitalPinToBitMask(PIN_CLOCK);
    source = ClockSource::NONE;
}

void Clock::set_source(enum ClockSource source) {
    if (source == this->source)
        return;

    noInterrupts();
    detachInterrupt(digitalPinToInterrupt(PIN_CLOCK));

    // MIDI clock -> port output
    if (source == ClockSource::MIDI) {
        pinMode(PIN_CLOCK, OUTPUT);
        write_output(false);
    }

    // Port input -> clock
    else if (source == ClockSource::EXT) {
        pinMode(PIN_CLOCK, INPUT);
        attachInterrupt(digitalPinToInterrupt(PIN_CLOCK), isr, RISING);
        ticks_counter = 0U;
        clock_event = false;
    }

    // Turn OFF clock completely
    else {
        pinMode(PIN_CLOCK, INPUT);
        write_output(false);
        ticks_counter = 0U;
        clock_event = false;
    }

    this->source = source;
    on_time = 0U;
    interrupts();
}

/**
 * @brief Counts midi ticks and sets output to ON if source is not NONE and N of ticks reached `divider` [0-4].
 * divider=0: 1/8 note, divider=1: 1/4 note, divider=2: 1/2 note, divider=3: whole note, divider=4: 2 notes.
 * NOTE: Will switch from EXT to MIDI mode automatically
 */
void Clock::midi_tick(void) {
    if (source == ClockSource::NONE)
        return;
    if (source == ClockSource::EXT)
        set_source(ClockSource::MIDI);

    uint64_t time = millis();
    midi_tick_time_last = time;

    // Count ticks
    if (ticks_counter < UINT8_MAX)
        ticks_counter++;
    else
        ticks_counter = 0U;
    if (ticks_counter >= ((uint8_t) 1 << divider) * 12U)
        ticks_counter = 0U;

    // Set event and clock output to ON
    if (ticks_counter == 0U) {
        clock_event = true;
        write_output(true);
    }
}

/**
 * @brief Handles in / out clock pulses and timeouts
 */
void Clock::loop(void) {
    // Handle external source ticks
    if (source == ClockSource::EXT) {
        boolean _clock_event_ext;
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { _clock_event_ext = clock_event_ext; }
        if (_clock_event_ext) {
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { clock_event_ext = false; }
            clock_event = true;
        }
    }

    if (source != ClockSource::MIDI)
        return;

    uint64_t time = millis();

    // Switch to external mode on timeout
    if (time - midi_tick_time_last > MIDI_CLOCK_TIMEOUT) {
        midi_tick_time_last = 0U;
        set_source(ClockSource::EXT);
    }

    // Clock output is currently ON -> check if it's time to turn it OFF
    if (on_time != 0U) {
        uint64_t time = millis();
        if (on_time > time || time - on_time >= CLOCK_HIGH_DURATION) {
            on_time = 0U;
            write_output(false);
        }
    }
}

/**
 * @brief Sets clock output ON or OFF considering CLOCK_INVERTED
 *
 * @param state true to set to ON
 */
void Clock::write_output(boolean state) {
#ifdef CLOCK_INVERTED
    if (state)
        *port_out_reg &= ~pin_mask;
    else
        *port_out_reg |= pin_mask;
#else
    if (state)
        *port_out_reg |= pin_mask;
    else
        *port_out_reg &= ~pin_mask;
#endif
}

/**
 * @brief Counts ticks into `ticks_counter_ext` and sets `clock_event_ext`
 */
void Clock::handle_interrupt(void) {
    if (ticks_counter_ext < UINT8_MAX)
        ticks_counter_ext++;
    else
        ticks_counter_ext = 0U;
    if (ticks_counter_ext >= ((uint8_t) 1 << divider))
        ticks_counter_ext = 0U;

    if (ticks_counter_ext == 0U)
        clock_event_ext = true;
}

/**
 * @brief Static interrupt callback (wrapper for `handle_interrupt()`)
 */
void Clock::isr(void) { clock.handle_interrupt(); }
