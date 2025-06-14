/**
 * @file pins.h
 * @author Fern Lane
 * @brief Hardware pins configuration
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

#ifndef PINS_H__
#define PINS_H__

#include <Arduino.h>

// ----------------------------- //
// 74HC595 shift-registers (DAC) //
// ----------------------------- //
const uint8_t PIN_DAC_LATCH_1 PROGMEM = 8U;
const uint8_t PIN_DAC_LATCH_2 PROGMEM = 9U;
const uint8_t PIN_DAC_LATCH_3 PROGMEM = 10U;

// ---------------------- //
// Trigger and gate ports //
// ---------------------- //
const uint8_t PIN_TRIG_1 PROGMEM = 5U;
const uint8_t PIN_GATE_1 PROGMEM = 4U;
const uint8_t PIN_TRIG_2 PROGMEM = 6U;
const uint8_t PIN_GATE_2 PROGMEM = 7U;

// -------------- //
// Clock in / out //
// -------------- //
// NOTE: Must be either 1 or 2 (pin with interrupt)
const uint8_t PIN_CLOCK PROGMEM = 2U;

// ----------- //
// WS2812 LEDs //
// ----------- //
const uint8_t PIN_LEDS PROGMEM = 3U;

// ------------------------------------------------ //
// Calibration start button and VCO test input port //
// ------------------------------------------------ //
// NOTE: PIN_CALIB_VCO must be either 1 or 2 (pin with interrupt)
const uint8_t PIN_CALIB_BTN PROGMEM = 12U;
const uint8_t PIN_CALIB_VCO PROGMEM = PIN_CLOCK;

// ------------ //
// DIP switches //
// ------------ //
// NOTE: Diodes must be on columns (see schematic for more info)
const uint8_t PINS_DIP_ROW[2] PROGMEM = {A1, A0};
const uint8_t PINS_DIP_COL[4] PROGMEM = {A2, A3, A4, A5};

#endif
