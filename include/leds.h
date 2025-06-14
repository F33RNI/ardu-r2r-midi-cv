/**
 * @file leds.h
 * @author Fern Lane
 * @brief 2x WS2812 status LEDs
 * Uses <https://github.com/adafruit/Adafruit_NeoPixel> library
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

#ifndef LEDS_H__
#define LEDS_H__

#include <Arduino.h>

#include <Adafruit_NeoPixel.h>

#include "calibration.h"

// Adafruit_NeoPixel library mode
#define LEDS_MODE NEO_GRB + NEO_KHZ800

// Blink modes: interval in ms, LEDs mask (00 / 01 / 10 / 11), ON color, OFF color
#define BLINK_CAL_PREP_GAIN_1      500U, 0b01U, 0x00000FU, 0U
#define BLINK_CAL_PREP_GAIN_2      500U, 0b10U, 0x00000FU, 0U
#define BLINK_CAL_PREP_TUNER       500U, 0b11U, 0x000A00U, 0U
#define BLINK_CAL_PREP_VCO_1       500U, 0b01U, 0x0A0A00U, 0U
#define BLINK_CAL_PREP_VCO_2       500U, 0b10U, 0x0A0A00U, 0U
#define BLINK_CAL_PREP_RESET_VCO_1 500U, 0b01U, 0x0F0000U, 0U
#define BLINK_CAL_PREP_RESET_VCO_2 500U, 0b10U, 0x0F0000U, 0U
#define BLINK_CAL_DONE             250U, 0b11U, 0x000F00U, 0U
#define BLINK_CAL_ERROR            250U, 0b11U, 0x0F0000U, 0U

// Colors
#define COLOR_INIT          0x010101U
#define COLOR_CAL_GAIN      0x00000FU
#define COLOR_CAL_VCO       0x0A0A00U
#define TUNER_BRIGHTNESS    45U
#define VCO_CAL_BRIGHTNESS  40U
#define NOTE_OFF_BRIGHTNESS 40U
#define NOTE_ON_BRIGHTNESS  60U
#define COLOR_CLOCK_BLINK   0x0C0C0CU

// Duration of clock blink in milliseconds
#define CLOCK_BLINK_DURATION 25U

class LEDs {
  public:
    void init(void);
    void loop(void);
    int16_t cents_1, cents_2, pitch_bend;

  private:
    Adafruit_NeoPixel leds;
    enum CalibStage cal_stage_last;
    uint64_t blink_timer, blink_interval;
    uint32_t blink_color_on, blink_color_off;
    boolean blink_state;
    uint8_t blink_mask;
    int16_t tuner_deviation_cent_last;
    uint8_t vco_calib_color_last;
    int16_t cents_1_last, cents_2_last;
    int16_t pitch_bend_last;
    boolean gate_1_last, gate_2_last;
    uint32_t color_norm_1_last, color_norm_2_last;

    void calibration_loop(void), normal_loop(void);
    void blink_start(uint64_t interval, uint8_t mask, uint32_t color_on, uint32_t color_off);
    void write(uint32_t color_1, uint32_t color_2, boolean reverse = false);
    void write(uint8_t color_1_r, uint8_t color_1_g, uint8_t color_1_b, uint8_t color_2_r, uint8_t color_2_g,
               uint8_t color_2_b, boolean reverse = false);
};

extern LEDs leds;

#endif
