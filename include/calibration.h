/**
 * @file calibration.h
 * @author Fern Lane
 * @brief DAC gain and automatic VCO-based linearity calibration
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

#ifndef CALIBRATION_H__
#define CALIBRATION_H__

#include <Arduino.h>

// Will print some debug info into serial port @ 115200 Bps
// NOTE: This may conflict with MIDI_SERIAL_INIT in "include/midi.h" file
// #define SERIAL_DEBUG
#ifdef SERIAL_DEBUG
#define DEBUG_INIT Serial.begin(115200UL)
#define DEBUG(x)   Serial.print(x)
#define DEBUGLN(x) Serial.println(x)
#else
#define DEBUG(x)
#define DEBUGLN(x)
#endif

// 1.1V internal reference value (adjust this if you have large supply voltage swings)
#define INTERNAL_VREF_MV 1100.f

// VCO's frequency filter K (0-1, closer to 1 - smoother but cal result in slow response and wrong reading)
#define CALIB_VCO_FREQ_FILTER_K 0.994f

// Calibration voltages (for calibrating DAC gains)
#define CALIB_DAC_TARGET_GAIN_1 3000.f
#define CALIB_DAC_TARGET_GAIN_2 3000.f

// Maximum allowed VCO note deviation to start calibration (in cents)
#define CALIB_VCO_START_DEV_CENTS 10

// Calibration button short and long press timings (in ms)
#define BTN_DEBOUNCE   240U
#define BTN_LONG_PRESS 1000U

// Delay before dropping voltage after frequency is within CALIB_VCO_START_DEV_CENTS range
#define CALIB_VCO_START_DELAY_INIT 5000U

// Delay before starting starting actual VCO calibration after dropping voltage to 0
#define CALIB_VCO_START_DELAY_LINEARITY 2000U

// Delay between voltage increments in milliseconds (more delay = slower but more precise)
#define CALIB_VCO_DELAY_BETWEEN_MV 10U

// Lowest voltage to start calibration from (in millivolts)
#define CALIB_VCO_MV_MIN 10.f

// % of maximum possible voltage for highest calibration voltage
#define CALIB_VCO_MAX_SCALE .95f

// EEPROM addresses
#define EEPROM_ADDR_GAIN_1   0
#define EEPROM_ADDR_GAIN_2   1
#define EEPROM_ADDR_MATRIX_1 2
#define EEPROM_ADDR_MATRIX_2 (EEPROM_ADDR_MATRIX_1 + sizeof(calibMatrix))

// How many vco note readings must be the same to consider this data point in calibration
#define VCO_LAST_CENTS_STAB 5U

// 2 bytes per note. Minimum note number is 12 (C0, 0mV)
// matrix[0] is note 12 (C0)
// matrix[115] is note 127 (G9)
struct calibMatrix {
    uint8_t note_min, note_max;
    uint16_t matrix[128 - 12];
};

enum class CalibStage : uint8_t {
    NONE,
    PREP_GAIN_1,
    GAIN_1,
    PREP_GAIN_2,
    GAIN_2,
    PREP_TUNER,
    TUNER,
    PREP_VCO_1,
    VCO_1,
    PREP_VCO_2,
    VCO_2,
    PREP_RESET_VCO_1,
    PREP_RESET_VCO_2,
    DONE,
    ERROR
};
enum class CalibVcoStage : uint8_t { NONE, TUNER, LOWER, LINEARITY };

class Calibration {
  public:
    void init_check(void);
    void loop(void);
    float note_to_mv_cal(uint8_t channel, uint16_t cents);
    boolean active;
    enum CalibStage stage;
    enum CalibVcoStage stage_vco;

    float gain_1_offset, gain_2_offset;
    int16_t tuner_deviation_cents;
    float vco_calib_progress;

  private:
    enum CalibStage stage_last;
    volatile uint8_t *btn_pin_in_reg;
    uint8_t btn_pin_mask;
    uint64_t btn_timer;
    boolean btn_handled;
    volatile uint64_t time_last;
    volatile float frequency_raw;
    float frequency, target_frequency;
    struct calibMatrix calib_matrix_1, calib_matrix_2;
    uint64_t vco_calib_timer;
    uint16_t mv_current, vco_cents_last, vco_cents_buffer[VCO_LAST_CENTS_STAB], vco_note_closest_mv;
    uint8_t vco_cents_buffer_counter;
    uint8_t address_last;

    void vco(void);
    void tuner(void);
    void read_matrices(void), write_matrices(void);
    void btn(void);
    void btn_short_press(void), btn_long_press(void);
    void handle_interrupt(void);
    static void isr(void);
};

extern Calibration calibration;

#endif
