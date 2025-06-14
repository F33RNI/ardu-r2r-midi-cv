/**
 * @file calibration.cpp
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

#include "include/calibration.h"
#include "include/dac.h"
#include "include/dip_switch.h"
#include "include/pins.h"
#include "include/utils.h"

#include <EEPROM.h>
#include <util/atomic.h>

// Preinstantiate
Calibration calibration;

/**
 * @brief Helper that sets right channel of DAC
 *
 * @param channel 0 - 1st channel, 1 - 2nd
 * @param mv target voltage in millivolts
 */
inline void set_dac_cv(uint8_t channel, float mv) {
    if (channel == 0)
        dac.set(mv, 0.f);
    else
        dac.set(0.f, mv);
}

/**
 * @brief Converts DIP switch bits into DAC gain offset
 *
 * @param dip_switch_state state of DIP switches (most significant bit = 1st dip switch)
 * @return float gain offset in +/- 0.127 range
 */
inline float dip_to_gain_offset(uint8_t dip_switch_state) {
    return (static_cast<float>(dip_switch_state & 0x7FU)) / ((dip_switch_state & 0x80U) ? 1e3f : -1e3f);
}

/**
 * @brief Reads calibration data from EEPROM, checks calibration button on startup and, if it pressed,
 * attaches VCO interrupt and sets `active` flag.
 */
void Calibration::init_check(void) {
#ifdef SERIAL_DEBUG
    DEBUG_INIT;
#endif

    // Initialize EEPROM and read offsets and matrices
    EEPROM.begin();
    delay(10);
    gain_1_offset = dip_to_gain_offset(EEPROM.read(EEPROM_ADDR_GAIN_1));
    gain_2_offset = dip_to_gain_offset(EEPROM.read(EEPROM_ADDR_GAIN_2));

    DEBUG(F("Gain offsets: "));
    DEBUG(gain_1_offset);
    DEBUG(",");
    DEBUGLN(gain_2_offset);

    read_matrices();

    // Initialize button for fast read and check if it pressed on startup
    pinMode(PIN_CALIB_BTN, INPUT_PULLUP);
    btn_pin_in_reg = portInputRegister(digitalPinToPort(PIN_CALIB_BTN));
    btn_pin_mask = digitalPinToBitMask(PIN_CALIB_BTN);
    delay(10);
    active = !(*btn_pin_in_reg & btn_pin_mask);

    // Nothing to do
    if (!active)
        return;

    // Ignore startup button press
    btn_handled = true;
    btn_timer = 1U;

    // Set first stage
    stage = CalibStage::PREP_GAIN_1;

    // Initialize VCO input
    detachInterrupt(digitalPinToInterrupt(PIN_CALIB_VCO));
    pinMode(PIN_CALIB_VCO, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_CALIB_VCO), isr, FALLING);
}

/**
 * @brief Converts MIDI note into DAC target considering calibration matrices.
 * NOTE: Call `read_matrices()` first
 *
 * @param channel 0 to use calib_matrix_1, 1 to use calib_matrix_2
 * @param cents MIDI note number in cents (12 is minimum allowed value). Ex.: 6000 - C4
 * @return float
 */
float Calibration::note_to_mv_cal(uint8_t channel, uint16_t cents) {
    // Check input range
    if (cents < 1200U || cents > 12700U)
        return 0.f;

    struct calibMatrix *matrix = channel ? &calib_matrix_2 : &calib_matrix_1;

    // No calibration stored or currently in calibration
    if (matrix->note_min > 127U || matrix->note_max > 127U || matrix->note_min == 0U ||
        matrix->note_min >= matrix->note_max || stage == CalibStage::PREP_VCO_1 || stage == CalibStage::PREP_VCO_2 ||
        stage == CalibStage::VCO_1 || stage == CalibStage::VCO_2)
        return note_to_mv(cents);

    float note = static_cast<float>(cents) / 100.f;

    uint8_t note_min, note_max;

    if (static_cast<uint8_t>(note) >= matrix->note_max) {
        note_min = matrix->note_max - 1U;
        note_max = matrix->note_max;
    } else if (static_cast<uint8_t>(note) <= matrix->note_min) {
        note_min = matrix->note_min;
        note_max = matrix->note_min + 1U;
    } else {
        note_min = note;
        note_max = static_cast<uint8_t>(note) + 1;
    }

    // Interpolate matrix
    return map_f(note, static_cast<float>(note_min), static_cast<float>(note_max),
                 static_cast<float>(matrix->matrix[note_min - 12U]),
                 static_cast<float>(matrix->matrix[note_max - 12U]));
}

/**
 * @brief Main calibration loop. Must be called inside `loop()` after reading DIP switches and before DAC write
 */
void Calibration::loop(void) {
    // Nothing to do here
    if (!active)
        return;
    if (stage == CalibStage::ERROR) {
        dac.set(0.f, 0.f);
        return;
    }

    // Read button
    btn();

    // Gain calibration -> read offset from DIP switch
    if (stage == CalibStage::GAIN_1)
        gain_1_offset = dip_to_gain_offset(dip_switch.states);
    else if (stage == CalibStage::GAIN_2)
        gain_2_offset = dip_to_gain_offset(dip_switch.states);

    // Filter VCO's frequency
    float _frequency_raw;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { _frequency_raw = frequency_raw; }
    if (frequency <= 1.f)
        frequency = _frequency_raw;
    else
        frequency = frequency * CALIB_VCO_FREQ_FILTER_K + _frequency_raw * (1.f - CALIB_VCO_FREQ_FILTER_K);

    // Check if it still measured (minimum is 0.5Hz)
    if (frequency > 0.f) {
        uint64_t _time_last;
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { _time_last = time_last; }
        uint64_t time = micros();
        if (time - _time_last > 2000000UL) {
            frequency = 0.f;
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { frequency_raw = 0.f; }
            if ((stage == CalibStage::VCO_1 || stage == CalibStage::VCO_2) && stage_vco == CalibVcoStage::LINEARITY) {
                stage = CalibStage::ERROR;
                return;
            }
        }
    }

    // Tuner
    if (stage == CalibStage::TUNER ||
        ((stage == CalibStage::VCO_1 || stage == CalibStage::VCO_2) && stage_vco == CalibVcoStage::TUNER))
        tuner();

    // VCO calibration
    if (stage == CalibStage::VCO_1 || stage == CalibStage::VCO_2)
        vco();
}

/**
 * @brief Tuner's loop
 */
void Calibration::tuner(void) {
    uint8_t target_octave = ((dip_switch.states & 0xF0) >> 4U);
    if (target_octave > 8U)
        target_octave = 8U;
    uint8_t target_note = dip_switch.states & 0x0F;
    if (target_note > 11U)
        target_note = 11U;
    uint16_t target_cents =
        (static_cast<uint16_t>(target_octave) * 12U + static_cast<uint16_t>(target_note) + 12U) * 100U;
    dac.set(note_to_mv_cal(0U, target_cents), note_to_mv_cal(1U, target_cents));
    target_frequency = note_to_hz(target_cents);
    tuner_deviation_cents = hz_to_cents_deviation(target_frequency, frequency);
}

/**
 * @brief VCO's calibration loop
 */
void Calibration::vco(void) {
    uint64_t time = millis();

    // 1st stage - check start frequency and wait 2s for it to settle before switching to the next stage
    if (stage_vco == CalibVcoStage::TUNER) {
        if (abs(tuner_deviation_cents) > CALIB_VCO_START_DEV_CENTS)
            vco_calib_timer = time;
        if (vco_calib_timer == 0U)
            vco_calib_timer = time;
        else if (vco_calib_timer > time || time - vco_calib_timer > CALIB_VCO_START_DELAY_INIT) {
            vco_calib_timer = 0U;
            stage_vco = CalibVcoStage::LOWER;
            set_dac_cv(stage == CalibStage::VCO_1 ? 0U : 1U, CALIB_VCO_MV_MIN);
            frequency = 0.f;
        }
    }

    // 2nd stage - wait for lowest frequency to settle
    else if (stage_vco == CalibVcoStage::LOWER) {
        if (vco_calib_timer == 0U)
            vco_calib_timer = time;
        else if (vco_calib_timer > time || time - vco_calib_timer > CALIB_VCO_START_DELAY_LINEARITY) {
            vco_calib_timer = 0U;

            for (uint8_t i = 0; i < VCO_LAST_CENTS_STAB; ++i)
                vco_cents_buffer[i] = 0U;
            vco_cents_buffer_counter = 0U;
            vco_cents_last = 0U;
            vco_note_closest_mv = static_cast<uint16_t>(CALIB_VCO_MV_MIN);
            mv_current = static_cast<uint16_t>(CALIB_VCO_MV_MIN);
            stage_vco = CalibVcoStage::LINEARITY;
            address_last = 255U;

            DEBUGLN(F("Voltage (mV),Note (cents),Comment"));
        }
    }

    // 3rd stage - actual calibration
    else if (stage_vco == CalibVcoStage::LINEARITY) {
        // Check time
        if (vco_calib_timer == 0U)
            vco_calib_timer = time;
        else if (vco_calib_timer < time && time - vco_calib_timer < CALIB_VCO_DELAY_BETWEEN_MV)
            return;

        uint16_t vco_cents = hz_to_note(frequency);

        // Wait for multiple frequency readings to be the same
        boolean stabilized = true;
        for (uint8_t i = 0; i < VCO_LAST_CENTS_STAB; ++i)
            if (vco_cents_buffer[i] == 0U ||
                abs(static_cast<int16_t>(vco_cents) - static_cast<int16_t>(vco_cents_buffer[i])) > 1U)
                stabilized = false;
        if (vco_cents_buffer_counter < VCO_LAST_CENTS_STAB - 1)
            vco_cents_buffer_counter++;
        else
            vco_cents_buffer_counter = 0U;
        vco_cents_buffer[vco_cents_buffer_counter] = vco_cents;
        if (!stabilized)
            return;

        // Reset timer
        vco_calib_timer = time;

        DEBUG(mv_current);
        DEBUG(",");
        DEBUG(vco_cents);
        DEBUG(",");

        // Check if we're in a valid range
        if (vco_cents >= 1200U && vco_cents <= 12700U && vco_cents_last >= 1200U && vco_cents_last <= 12700U) {
            uint8_t vco_note = static_cast<uint8_t>(vco_cents / 100U);
            uint8_t vco_note_last = static_cast<uint8_t>(vco_cents_last / 100U);

            // New closest voltage found
            if (vco_note == vco_note_last && vco_cents % 100U < vco_cents_last % 100U)
                vco_note_closest_mv = mv_current;

            // New note -> save to matrix
            if (vco_note > vco_note_last) {
                struct calibMatrix *matrix = stage == CalibStage::VCO_1 ? &calib_matrix_1 : &calib_matrix_2;

                if (matrix->note_min > 127U) {
                    matrix->note_min = static_cast<uint8_t>(vco_note_last);

                    DEBUG(F("MIN: "));
                    DEBUG(matrix->note_min);
                }

                uint8_t address = static_cast<uint8_t>(vco_note_last - 12U);

                // Check address before writing (currently, cannot handle +2 or more notes)
                if (address_last != 255U &&
                    (address < address_last || static_cast<uint8_t>(address - address_last) > 1U)) {

                    DEBUGLN();
                    DEBUG("ERROR: ");
                    DEBUG(address_last);
                    DEBUG(" -> ");
                    DEBUGLN(address);

                    stage = CalibStage::ERROR;
                    return;
                }

                // Prevent writing multiple times to the same address due to possible noise
                if (address_last == 255U || address > address_last) {
                    matrix->matrix[address] = vco_note_closest_mv;
                    address_last = address;

                    DEBUG(F(" matrix["));
                    DEBUG(address);
                    DEBUG(F("] = "));
                    DEBUG(vco_note_closest_mv);

                    vco_note_closest_mv = mv_current;
                }
            }
        }

        // Out of range
        else
            vco_note_closest_mv = mv_current;

        // Increment and set voltage
        mv_current++;
        set_dac_cv(stage == CalibStage::VCO_1 ? 0U : 1U, static_cast<float>(mv_current));

        // Cents buffer
        for (uint8_t i = 0; i < VCO_LAST_CENTS_STAB; ++i)
            vco_cents_buffer[i] = 0U;
        vco_cents_buffer_counter = 0U;

        // Calculate progress (for LED)
        float mv_end = dac.get_current_maximum(stage == CalibStage::VCO_1 ? 0U : 1U) * CALIB_VCO_MAX_SCALE;
        float progress_1 = static_cast<float>(vco_cents) / 12700.f;
        float progress_2 = static_cast<float>(mv_current) / mv_end;
        vco_calib_progress = max(progress_1, progress_2);

        // Calibration done
        if (static_cast<float>(mv_current) >= mv_end || vco_cents > 12700U) {
            struct calibMatrix *matrix = stage == CalibStage::VCO_1 ? &calib_matrix_1 : &calib_matrix_2;
            matrix->note_max = static_cast<uint8_t>(vco_cents_last / 100U);
            matrix->matrix[matrix->note_max - 12U] = vco_note_closest_mv;
            write_matrices();

            DEBUG(F("MAX: "));
            DEBUG(matrix->note_max);
            DEBUG(F(" matrix["));
            DEBUG(matrix->note_max - 12U);
            DEBUG(F("] = "));
            DEBUGLN(vco_note_closest_mv);

            vco_calib_timer = 0U;
            stage_vco = CalibVcoStage::TUNER;
            stage_last = stage;
            stage = CalibStage::DONE;
            dac.set(0.f, 0.f);
        }

        vco_cents_last = vco_cents;

        DEBUGLN();
    }
}

/**
 * @brief Handles calibration button short and long presses.
 * Short press -> next stage, Long press -> start VCO calibration / write DAC gain calibration / reset VCO calibration
 */
void Calibration::btn(void) {
    uint64_t time = millis();

    // Handle millis() overflow
    if (btn_timer > time) {
        btn_timer = time;
    }

    // Fast read
    boolean btn_pressed = !(*btn_pin_in_reg & btn_pin_mask);

    // Button just pressed
    if (btn_pressed && btn_timer == 0U) {
        btn_timer = time;
        btn_handled = false;
    }

    // Button released -> check debounce and if no btn_handled flag and handle it as a short press
    if (!btn_pressed && btn_timer != 0U && time - btn_timer >= BTN_DEBOUNCE) {
        btn_timer = 0U;
        if (!btn_handled) {
            btn_handled = true;
            btn_short_press();
        }
    }

    // Button pressed for a long time -> handle it as a long press
    if (btn_pressed && !btn_handled && btn_timer != 0U && time - btn_timer >= BTN_LONG_PRESS) {
        btn_handled = true;
        btn_long_press();
    }
}

/**
 * @brief Handles short button press (cycles trough calibration stages)
 */
void Calibration::btn_short_press(void) {
    switch (stage) {
    // Next stage
    // 1st DAC gain calibration -> 2nd DAC gain calibration
    case CalibStage::PREP_GAIN_1:
        stage = CalibStage::PREP_GAIN_2;
        break;
    // 2nd DAC gain calibration -> Tuner
    case CalibStage::PREP_GAIN_2:
        stage = CalibStage::PREP_TUNER;
        break;
    // Tuner -> VCO 1 calibration
    case CalibStage::PREP_TUNER:
        stage = CalibStage::PREP_VCO_1;
        break;
    // VCO 1 calibration -> VCO 2 calibration
    case CalibStage::PREP_VCO_1:
        stage = CalibStage::PREP_VCO_2;
        break;
    // VCO 2 calibration -> VCO 1 calibration reset
    case CalibStage::PREP_VCO_2:
        stage = CalibStage::PREP_RESET_VCO_1;
        break;
    // VCO 1 calibration reset -> VCO 2 calibration reset
    case CalibStage::PREP_RESET_VCO_1:
        stage = CalibStage::PREP_RESET_VCO_2;
        break;
    // VCO 2 calibration reset -> 1.1 REF calibration
    case CalibStage::PREP_RESET_VCO_2:
        stage = CalibStage::PREP_GAIN_1;
        break;

    // After VCO calibration
    case CalibStage::DONE:
        stage = stage_last == CalibStage::VCO_1 ? CalibStage::PREP_VCO_2 : CalibStage::PREP_RESET_VCO_1;
        break;
    default:
        break;
    }
    dac.set(0.f, 0.f);
}

/**
 * @brief Handles long button press (starts calibration)
 */
void Calibration::btn_long_press(void) {
    switch (stage) {
    // Start DAC gain calibration
    case CalibStage::PREP_GAIN_1:
        dac.set(CALIB_DAC_TARGET_GAIN_1, 0.f);
        stage = CalibStage::GAIN_1;
        break;
    case CalibStage::PREP_GAIN_2:
        dac.set(0.f, CALIB_DAC_TARGET_GAIN_2);
        stage = CalibStage::GAIN_2;
        break;

    // Confirm and write DAC gain calibration and go to the next stage
    case CalibStage::GAIN_1:
        EEPROM.write(EEPROM_ADDR_GAIN_1, dip_switch.states);
        dac.set(0.f, 0.f);
        stage = CalibStage::PREP_GAIN_2;
        break;
    case CalibStage::GAIN_2:
        EEPROM.write(EEPROM_ADDR_GAIN_2, dip_switch.states);
        dac.set(0.f, 0.f);
        stage = CalibStage::PREP_TUNER;
        break;

    // Start tuner
    case CalibStage::PREP_TUNER:
        tuner_deviation_cents = 0;
        frequency = 0.f;
        stage = CalibStage::TUNER;
        break;

    // Exit from tuner and go to the next stage
    case CalibStage::TUNER:
        tuner_deviation_cents = 0;
        stage = CalibStage::PREP_VCO_1;
        dac.set(0.f, 0.f);
        break;

    // Start VCO calibration
    case CalibStage::PREP_VCO_1:
    case CalibStage::PREP_VCO_2:
        (stage == CalibStage::PREP_VCO_1 ? calib_matrix_1 : calib_matrix_2).note_min = 255U;
        (stage == CalibStage::PREP_VCO_1 ? calib_matrix_1 : calib_matrix_2).note_max = 255U;
        stage = (stage == CalibStage::PREP_VCO_1 ? CalibStage::VCO_1 : CalibStage::VCO_2);
        stage_vco = CalibVcoStage::TUNER;
        frequency = 0.f;
        vco_calib_timer = 0U;
        vco_calib_progress = 0.f;
        break;

    // Reset stored VCO calibration and go to the next stage
    case CalibStage::PREP_RESET_VCO_1:
    case CalibStage::PREP_RESET_VCO_2:
        (stage == CalibStage::PREP_VCO_1 ? calib_matrix_1 : calib_matrix_2).note_min = 255U;
        (stage == CalibStage::PREP_VCO_1 ? calib_matrix_1 : calib_matrix_2).note_max = 255U;
        write_matrices();
        stage = (stage == CalibStage::PREP_RESET_VCO_1 ? CalibStage::PREP_RESET_VCO_2 : CalibStage::PREP_GAIN_1);
        break;
    default:
        break;
    }
}

/**
 * @brief Reads calib_matrix_1 and calib_matrix_2 from EEPROM
 */
void Calibration::read_matrices(void) {
    EEPROM.get<calibMatrix>(EEPROM_ADDR_MATRIX_1, calib_matrix_1);
    EEPROM.get<calibMatrix>(EEPROM_ADDR_MATRIX_2, calib_matrix_2);

#ifdef SERIAL_DEBUG
    DEBUGLN(F("\n--- MATRIX 1 ---"));
    DEBUG(F("MIN NOTE: "));
    DEBUGLN(calib_matrix_1.note_min);
    DEBUG(F("MAX NOTE: "));
    DEBUGLN(calib_matrix_1.note_max);
    DEBUGLN(F("----------------"));
    DEBUGLN(F("Note,Target voltage (mv)"));
    if (calib_matrix_1.note_min < 127U && calib_matrix_1.note_max < 127U)
        for (uint8_t i = calib_matrix_1.note_min - 12U; i < calib_matrix_1.note_max - 12U; ++i) {
            DEBUG(i + 12U);
            DEBUG(",");
            DEBUGLN(calib_matrix_1.matrix[i]);
        }
    DEBUGLN(F("\n--- MATRIX 2 ---"));
    DEBUG(F("MIN NOTE: "));
    DEBUGLN(calib_matrix_2.note_min);
    DEBUG(F("MAX NOTE: "));
    DEBUGLN(calib_matrix_2.note_max);
    DEBUGLN(F("----------------"));
    DEBUGLN(F("Note,Target voltage (mv)"));
    if (calib_matrix_2.note_min < 127U && calib_matrix_2.note_max < 127U)
        for (uint8_t i = calib_matrix_2.note_min - 12U; i < calib_matrix_2.note_max - 12U; ++i) {
            DEBUG(i + 12U);
            DEBUG(",");
            DEBUGLN(calib_matrix_2.matrix[i]);
        }
    DEBUGLN();
#endif
}

/**
 * @brief Writes calib_matrix_1 and calib_matrix_2 into EEPROM
 */
void Calibration::write_matrices(void) {
    EEPROM.put<calibMatrix>(EEPROM_ADDR_MATRIX_1, calib_matrix_1);
    EEPROM.put<calibMatrix>(EEPROM_ADDR_MATRIX_2, calib_matrix_2);
}

/**
 * @brief Measures raw VCO's frequency using time between interrupts
 */
void Calibration::handle_interrupt(void) {
    uint64_t time = micros();

    // Handle micros() overflow
    if (time_last == 0U || time_last >= time) {
        time_last = time;
        return;
    }

    // Calculate unfiltered frequency
    frequency_raw = 1e6f / static_cast<float>(time - time_last);
    time_last = time;
}

/**
 * @brief Static interrupt callback (wrapper for handle_interrupt())
 */
void Calibration::isr(void) { calibration.handle_interrupt(); }
