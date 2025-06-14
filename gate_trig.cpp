/**
 * @file gate_trig.cpp
 * @author Fern Lane
 * @brief Gate / trig ports handler
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

#include "include/gate_trig.h"
#include "include/pins.h"

// Preinstantiate
GateTrig gate_trig;

/**
 * @brief Sets up output ports
 */
void GateTrig::init(void) {
    gate_1_out_reg = portOutputRegister(digitalPinToPort(PIN_GATE_1));
    gate_2_out_reg = portOutputRegister(digitalPinToPort(PIN_GATE_2));
    trig_1_out_reg = portOutputRegister(digitalPinToPort(PIN_TRIG_1));
    trig_2_out_reg = portOutputRegister(digitalPinToPort(PIN_TRIG_2));
    gate_1_pin_mask = digitalPinToBitMask(PIN_GATE_1);
    gate_2_pin_mask = digitalPinToBitMask(PIN_GATE_2);
    trig_1_pin_mask = digitalPinToBitMask(PIN_TRIG_1);
    trig_2_pin_mask = digitalPinToBitMask(PIN_TRIG_2);
    pinMode(PIN_GATE_1, OUTPUT);
    pinMode(PIN_GATE_2, OUTPUT);
    pinMode(PIN_TRIG_1, OUTPUT);
    pinMode(PIN_TRIG_2, OUTPUT);
    gate_1_write(false);
    gate_2_write(false);
    trig_1_write(false);
    trig_2_write(false);
}

/**
 * @brief Stops trig pulse after TRIG_1_DURATION / TRIG_2_DURATION
 */
void GateTrig::loop(void) {
    if (trig_1_timer == 0U && trig_2_timer == 0U)
        return;

    uint64_t time = millis();

    if (trig_1_timer > time || time - trig_1_timer >= TRIG_1_DURATION) {
        trig_1_write(false);
        trig_1_timer = 0;
    }
    if (trig_2_timer > time || time - trig_2_timer >= TRIG_2_DURATION) {
        trig_2_write(false);
        trig_2_timer = 0;
    }
}

/**
 * @brief Sets gate output and starts trigger (or re-trigger)
 *
 * @param state true to ON, false to OFF
 * @param from_self internal parameter, used for merging gates and triggers
 */
void GateTrig::set_1(boolean state, boolean from_self) {
    gate_1_write(state);
    if (state) {
        trig_1_write(true);
        trig_1_timer = millis();
    }
    if (!from_self && merged)
        set_2(state, true);
}

/**
 * @brief Sets gate output and starts trigger (or re-trigger)
 *
 * @param state true to ON, false to OFF
 * @param from_self internal parameter, used for merging gates and triggers
 */
void GateTrig::set_2(boolean state, boolean from_self) {
    gate_2_write(state);
    if (state) {
        trig_2_write(true);
        trig_2_timer = millis();
    }
    if (!from_self && merged)
        set_1(state, true);
}

/**
 * @brief Writes gate 1 port
 *
 * @param state true to ON, false to OFF
 */
void GateTrig::gate_1_write(boolean state) {
    gate_1_state = state;
#ifdef GATE_1_INVERTED
    if (state)
        *gate_1_out_reg &= ~gate_1_pin_mask;
    else
        *gate_1_out_reg |= gate_1_pin_mask;
#else
    if (state)
        *gate_1_out_reg |= gate_1_pin_mask;
    else
        *gate_1_out_reg &= ~gate_1_pin_mask;
#endif
}

/**
 * @brief Writes gate 2 port
 *
 * @param state true to ON, false to OFF
 */
void GateTrig::gate_2_write(boolean state) {
    gate_2_state = state;
#ifdef GATE_2_INVERTED
    if (state)
        *gate_2_out_reg &= ~gate_2_pin_mask;
    else
        *gate_2_out_reg |= gate_2_pin_mask;
#else
    if (state)
        *gate_2_out_reg |= gate_2_pin_mask;
    else
        *gate_2_out_reg &= ~gate_2_pin_mask;
#endif
}

/**
 * @brief Writes trig 1 port
 *
 * @param state true to ON, false to OFF
 */
void GateTrig::trig_1_write(boolean state) {
#ifdef TRIG_1_INVERTED
    if (state)
        *trig_1_out_reg &= ~trig_1_pin_mask;
    else
        *trig_1_out_reg |= trig_1_pin_mask;
#else
    if (state)
        *trig_1_out_reg |= trig_1_pin_mask;
    else
        *trig_1_out_reg &= ~trig_1_pin_mask;
#endif
}

/**
 * @brief Writes trig 2 port
 *
 * @param state true to ON, false to OFF
 */
void GateTrig::trig_2_write(boolean state) {
#ifdef TRIG_2_INVERTED
    if (state)
        *trig_2_out_reg &= ~trig_2_pin_mask;
    else
        *trig_2_out_reg |= trig_2_pin_mask;
#else
    if (state)
        *trig_2_out_reg |= trig_2_pin_mask;
    else
        *trig_2_out_reg &= ~trig_2_pin_mask;
#endif
}
