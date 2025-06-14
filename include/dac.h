/**
 * @file dac.h
 * @author Fern Lane
 * @brief R2R 74HC595 shift-register -based 2x12 bit DAC
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

#ifndef DAC_H__
#define DAC_H__

#include <Arduino.h>

// 12 bit
#define DAC_MAX 4095U

// Base (rough) DAC amplifier gains (user can calibrate +/- 0.127). Depends on R13-R16 (see schematic).
// Change these values if you have different resistors / out of range during calibration.
// Example: if R13 = 7K5 and R14 = 10K, then GAIN_1_BASE = 1 + (7.5 / 10) = 1.75.
// TODO: Change to 1.75f
const float GAIN_1_BASE PROGMEM = 1.824f;
const float GAIN_2_BASE PROGMEM = 1.824f;

class DAC {
  public:
    void init(void);
    void set(float target_1, float target_2);
    void write(void);
    void calculate_compensation(void);
    float get_current_maximum(uint8_t dac);

  private:
    volatile uint8_t *dac_1_port_out_reg, *dac_2_port_out_reg, *dac_3_port_out_reg;
    uint8_t dac_1_mask, dac_2_mask, dac_3_mask;
    uint16_t dac_1_value, dac_2_value;
    float vcc, dac_1_target, dac_2_target;
    uint16_t vcc_raw;
};

extern DAC dac;

#endif
