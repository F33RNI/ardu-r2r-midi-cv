/**
 * @file gate_trig.h
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

#ifndef GATE_TRIG_H__
#define GATE_TRIG_H__

#include <Arduino.h>

// Uncomment to invert some ports (to use LOW as ON instead of HIGH)
// #define GATE_1_INVERTED
// #define GATE_2_INVERTED
// #define TRIG_1_INVERTED
// #define TRIG_2_INVERTED

// Duration of trig pulses (in milliseconds)
#define TRIG_1_DURATION 10U
#define TRIG_2_DURATION 10U

class GateTrig {
  public:
    void init(void);
    void loop(void);
    void set_1(boolean state, boolean from_self = false), set_2(boolean state, boolean from_self = false);
    boolean merged;
    boolean gate_1_state, gate_2_state;

  private:
    volatile uint8_t *gate_1_out_reg, *gate_2_out_reg, *trig_1_out_reg, *trig_2_out_reg;
    uint8_t gate_1_pin_mask, gate_2_pin_mask, trig_1_pin_mask, trig_2_pin_mask;
    uint64_t trig_1_timer, trig_2_timer;

    void gate_1_write(boolean state), gate_2_write(boolean state);
    void trig_1_write(boolean state), trig_2_write(boolean state);
};

extern GateTrig gate_trig;

#endif
