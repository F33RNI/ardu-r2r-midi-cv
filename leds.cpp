/**
 * @file leds.cpp
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

#include "include/leds.h"
#include "include/calibration.h"
#include "include/clock.h"
#include "include/gate_trig.h"
#include "include/pins.h"
#include "include/utils.h"

// Preinstantiate
LEDs leds;

/**
 * @brief Converts WRGB uint32_t color into RGB components
 *
 * @param color WRGB (W ignored)
 * @param r red channel
 * @param g green channel
 * @param b blue channel
 */
static inline void unpack_rgb(uint32_t color, uint8_t &r, uint8_t &g, uint8_t &b) {
    r = static_cast<uint8_t>((color >> 16U) & 0xFFU);
    g = static_cast<uint8_t>((color >> 8U) & 0xFFU);
    b = static_cast<uint8_t>(color & 0xFFU);
}

/**
 * @brief Initializes library's instance
 */
void LEDs::init(void) {
    this->leds = Adafruit_NeoPixel(2U, PIN_LEDS, LEDS_MODE);
    this->leds.begin();
    write(COLOR_INIT, COLOR_INIT);

    cal_stage_last = CalibStage::NONE;
}

/**
 * @brief Calculates LEDs color, handles blinking and writes everything to LEDs.
 * NOTE: This must be called in main `loop()` as fast as possible
 */
void LEDs::loop(void) {
    uint64_t time = millis();

    if (calibration.active)
        calibration_loop();
    else
        normal_loop();

    // Handle blinking
    if (blink_interval) {
        // Handle millis() overflow
        if (blink_timer == 0U || blink_timer > time) {
            blink_timer = time;
            return;
        }

        // Toggle colors
        if (time - blink_timer >= blink_interval) {
            blink_timer = time;
            blink_state = !blink_state;
            write(blink_mask & 0b01 ? (blink_state ? blink_color_on : blink_color_off) : 0U,
                  blink_mask & 0b10 ? (blink_state ? blink_color_on : blink_color_off) : 0U);
        }
    }
}

/**
 * @brief Calibration mode
 */
void LEDs::calibration_loop(void) {
    if (calibration.stage != cal_stage_last) {
        cal_stage_last = calibration.stage;
        switch (calibration.stage) {
        // Prepare
        case CalibStage::PREP_GAIN_1:
            blink_start(BLINK_CAL_PREP_GAIN_1);
            break;
        case CalibStage::PREP_GAIN_2:
            blink_start(BLINK_CAL_PREP_GAIN_2);
            break;
        case CalibStage::PREP_TUNER:
            blink_start(BLINK_CAL_PREP_TUNER);
            break;
        case CalibStage::PREP_VCO_1:
            blink_start(BLINK_CAL_PREP_VCO_1);
            break;
        case CalibStage::PREP_VCO_2:
            blink_start(BLINK_CAL_PREP_VCO_2);
            break;
        case CalibStage::PREP_RESET_VCO_1:
            blink_start(BLINK_CAL_PREP_RESET_VCO_1);
            break;
        case CalibStage::PREP_RESET_VCO_2:
            blink_start(BLINK_CAL_PREP_RESET_VCO_2);
            break;

        // DAC gain calibration
        case CalibStage::GAIN_1:
        case CalibStage::GAIN_2:
            blink_interval = 0U;
            write(COLOR_CAL_GAIN, 0U, calibration.stage == CalibStage::GAIN_2);
            break;

        // Tuner
        case CalibStage::TUNER:
            blink_interval = 0U;
            write(0U, 0U);
            break;

        // VCO calibration
        case CalibStage::VCO_1:
        case CalibStage::VCO_2:
            blink_interval = 0U;
            write(COLOR_CAL_VCO, 0U, calibration.stage == CalibStage::VCO_2);
            break;

        // Done and error
        case CalibStage::DONE:
            blink_start(BLINK_CAL_DONE);
            break;
        case CalibStage::ERROR:
            blink_start(BLINK_CAL_ERROR);
            break;
        default:
            break;
        }
    }

    // Tuner
    if (calibration.stage == CalibStage::TUNER ||
        ((calibration.stage == CalibStage::VCO_1 || calibration.stage == CalibStage::VCO_2) &&
         calibration.stage_vco == CalibVcoStage::TUNER)) {
        if (calibration.tuner_deviation_cents != tuner_deviation_cent_last) {
            tuner_deviation_cent_last = calibration.tuner_deviation_cents;
            if (calibration.tuner_deviation_cents >= -250 && calibration.tuner_deviation_cents <= 250) {
                uint8_t red = map_uint8(static_cast<uint8_t>(abs(calibration.tuner_deviation_cents)), 0U, 250U, 0U,
                                        TUNER_BRIGHTNESS);
                uint8_t green = map_uint8(static_cast<uint8_t>(abs(calibration.tuner_deviation_cents)), 0U, 250U, 1U,
                                          TUNER_BRIGHTNESS - red);
                write(0U, (green > 1U ? 0U : 1U), 0U, red, green, 0U, calibration.tuner_deviation_cents < 0);
            } else
                write(TUNER_BRIGHTNESS, 0U, 0U, 0U, 0U, 0U, calibration.tuner_deviation_cents > 0);
        }
    }

    // VCO
    else if (calibration.stage == CalibStage::VCO_1 || calibration.stage == CalibStage::VCO_2) {
        if (calibration.stage_vco == CalibVcoStage::LOWER)
            write(COLOR_CAL_VCO, 0U, calibration.stage == CalibStage::VCO_2);
        else if (calibration.stage_vco == CalibVcoStage::LINEARITY) {
            uint8_t blue =
                static_cast<uint8_t>(map_f(calibration.vco_calib_progress, 0.f, 1.f, 0.f, VCO_CAL_BRIGHTNESS));
            if (blue != vco_calib_color_last) {
                vco_calib_color_last = blue;
                uint8_t green = VCO_CAL_BRIGHTNESS - blue;
                write(0U, green, blue, 0U, 0U, 0U, calibration.stage == CalibStage::VCO_2);
            }
        }
    }
}

/**
 * @brief Normal mode. NOTE: `clock.clock_event` must be cleared outside this function
 */
void LEDs::normal_loop(void) {
    uint64_t time = millis();

    // Clock blink
    blink_interval = 0U;
    if (clock.clock_event) {
        blink_timer = time;
        blink_state = false;
    }
    if (blink_timer != 0U && blink_timer <= time && time - blink_timer < CLOCK_BLINK_DURATION) {
        if (!blink_state) {
            write(COLOR_CLOCK_BLINK, COLOR_CLOCK_BLINK);
            blink_state = true;
        }
        return;
    }

    // Restore normal state
    if (blink_state) {
        blink_state = false;
        blink_timer = 0U;
        write(color_norm_1_last, color_norm_2_last);
    }

    // Show pitch & gate states
    for (uint8_t i = 0; i < 2U; ++i) {
        int16_t &cents = (i ? cents_2 : cents_1);
        int16_t &cents_last = (i ? cents_2_last : cents_1_last);
        boolean &gate = (i ? gate_trig.gate_2_state : gate_trig.gate_1_state);
        boolean &gate_last = (i ? gate_2_last : gate_1_last);

        if (cents == cents_last && pitch_bend == pitch_bend_last && gate == gate_last)
            continue;

        uint16_t cents_ = static_cast<uint16_t>(cents + pitch_bend);
        if (cents_ > 12700U)
            cents_ = 12700U;
        uint32_t color = this->leds.gamma32(
            this->leds.ColorHSV(cents_ * 5U, 255U, (gate ? NOTE_ON_BRIGHTNESS : NOTE_OFF_BRIGHTNESS)));

        write(color, (i ? color_norm_1_last : color_norm_2_last), !(i == 0));

        cents_last = cents;
        gate_last = gate;
        (i ? color_norm_2_last : color_norm_1_last) = color;
    }
}

/**
 * @brief Starts blinking
 *
 * @param interval blinking interval in ms
 * @param mask which LEDs to blink (00, 01, 10, 11). Least significant bit is 1st LED (address 0)
 * @param color_on ON color (32-bit, WRGB)
 * @param color_off OFF color (32-bit, WRGB)
 */
void LEDs::blink_start(uint64_t interval, uint8_t mask, uint32_t color_on, uint32_t color_off) {
    blink_state = false;
    blink_interval = interval;
    blink_mask = mask;
    blink_color_on = color_on;
    blink_color_off = color_off;
    write(blink_color_off, blink_color_off);
    blink_timer = millis();
}

/**
 * @brief Calls `setPixelColor()` and `show()`
 *
 * @param color_1 1st LED color in WRGB order
 * @param color_2 2nd LED color in WRGB order
 * @param reverse true to write 1st LED's color into 2nd LED and 2nd LED's color to the 1st one (reverse direction)
 */
void LEDs::write(uint32_t color_1, uint32_t color_2, boolean reverse) {
    this->leds.setPixelColor(reverse ? 1U : 0U, color_1);
    this->leds.setPixelColor(reverse ? 0U : 1U, color_2);
    this->leds.show();
}

/**
 * @brief Calls `setPixelColor()` and `show()`
 *
 * @param color_1_r 1st LED red channel value
 * @param color_1_g 1st LED green channel value
 * @param color_1_b 1st LED blue channel value
 * @param color_2_r 2nd LED red channel value
 * @param color_2_g 2nd LED green channel value
 * @param color_2_b 2nd LED blue channel value
 * @param reverse true to write 1st LED's color into 2nd LED and 2nd LED's color to the 1st one (reverse direction)
 */
void LEDs::write(uint8_t color_1_r, uint8_t color_1_g, uint8_t color_1_b, uint8_t color_2_r, uint8_t color_2_g,
                 uint8_t color_2_b, boolean reverse) {
    this->leds.setPixelColor(reverse ? 1U : 0U, color_1_r, color_1_g, color_1_b);
    this->leds.setPixelColor(reverse ? 0U : 1U, color_2_r, color_2_g, color_2_b);
    this->leds.show();
}
