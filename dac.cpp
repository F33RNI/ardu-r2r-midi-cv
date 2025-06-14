/**
 * @file dac.cpp
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

#include "include/dac.h"
#include "include/calibration.h"
#include "include/pins.h"
#include "include/utils.h"

#include <SPI.h>

// Preinstantiate
DAC dac;

/**
 * @brief Initialised DAC pins, SPI as master and ADC to measure VCC
 */
void DAC::init(void) {
    // Latches
    pinMode(PIN_DAC_LATCH_1, OUTPUT);
    pinMode(PIN_DAC_LATCH_2, OUTPUT);
    pinMode(PIN_DAC_LATCH_3, OUTPUT);

    // Store ports and masks for fast digital write
    dac_1_port_out_reg = portOutputRegister(digitalPinToPort(PIN_DAC_LATCH_1));
    dac_2_port_out_reg = portOutputRegister(digitalPinToPort(PIN_DAC_LATCH_2));
    dac_3_port_out_reg = portOutputRegister(digitalPinToPort(PIN_DAC_LATCH_3));
    dac_1_mask = digitalPinToBitMask(PIN_DAC_LATCH_1);
    dac_2_mask = digitalPinToBitMask(PIN_DAC_LATCH_2);
    dac_3_mask = digitalPinToBitMask(PIN_DAC_LATCH_3);

    // Initialize SPI as master
    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    SPI.setClockDivider(SPI_CLOCK_DIV2);

    // Make first write
    write();

    // Set the Vref to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
#else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

    // Wait for VRef to settle
    delay(20);

    // Make first VCC reading
    calculate_compensation();
    write();
}

/**
 * @brief Sets DAC target output voltages without doing any actual writes to DAC
 *
 * @param target_1 1st channel target in mV. NAN to not change
 * @param target_2 2nd channel target in mV. NAN to not change
 */
void DAC::set(float target_1, float target_2) {
    if (!isnanf(target_1))
        dac_1_target = target_1 >= 0.f ? target_1 : 0.f;
    if (!isnanf(target_2))
        dac_2_target = target_2 >= 0.f ? target_2 : 0.f;
}

/**
 * @brief Writes calculated DAC values using SPI.
 * NOTE: This must be called in `loop()` immediately after calculate_compensation()
 */
void DAC::write(void) {
    // Send lowest 8 bits of first dac value
    *dac_1_port_out_reg &= ~dac_1_mask;
    SPI.transfer(dac_1_value & 0xFF);
    *dac_1_port_out_reg |= dac_1_mask;

    // Send highest 4 bits of first dac value and lowest 4 bits of second dac value
    *dac_2_port_out_reg &= ~dac_2_mask;
    SPI.transfer(((dac_1_value >> 8) & 0xFF) | (((dac_2_value & 0b00001111) << 4) & 0xFF));
    *dac_2_port_out_reg |= dac_2_mask;

    // Send highest 8 bits of second dac value
    *dac_3_port_out_reg &= ~dac_3_mask;
    SPI.transfer((dac_2_value >> 4) & 0xFF);
    *dac_3_port_out_reg |= dac_3_mask;
}

/**
 * @brief Measures VCC and calculates compensated raw DAC values considering DAC gains (and offsets from calibration).
 * NOTE: This must be called in `loop()` before `write()` and as fast as possible
 */
void DAC::calculate_compensation(void) {
    // Start ADC conversion and measure 1.1V reference against AVcc
    ADCSRA |= _BV(ADSC);
    while (bit_is_set(ADCSRA, ADSC))
        ;

    // Read result and calculate VCC in millivolts
    vcc_raw = ADCL | (ADCH << 8);
    vcc = (INTERNAL_VREF_MV * 1023.f) / static_cast<float>(vcc_raw);

    // Calculate gains and compensate for VCC
    dac_1_value = map_f(dac_1_target, 0.f, vcc * (GAIN_1_BASE + calibration.gain_1_offset), 0.f, DAC_MAX);
    dac_2_value = map_f(dac_2_target, 0.f, vcc * (GAIN_2_BASE + calibration.gain_2_offset), 0.f, DAC_MAX);

    // Clamp to maximum possible value
    if (dac_1_value > DAC_MAX)
        dac_1_value = DAC_MAX;
    if (dac_2_value > DAC_MAX)
        dac_2_value = DAC_MAX;
}

/**
 * @brief Calculates current highest possible output voltage.
 * NOTE: call calculate_compensation() before it at least ones to measure VCC
 *
 * @param dac 0 to calculate for 1st DAC, 1 for 2nd
 * @return float maximum possible output voltage in millivolts
 */
float DAC::get_current_maximum(uint8_t dac) {
    return vcc * ((dac ? GAIN_2_BASE : GAIN_1_BASE) + (dac ? calibration.gain_2_offset : calibration.gain_1_offset));
}
