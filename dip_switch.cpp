/**
 * @file dip_switch.cpp
 * @author Fern Lane
 * @brief 2x4 6-wires DIP switch reader (see schematic for more info)
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

#include "include/dip_switch.h"
#include "include/pins.h"

// Preinstantiate
DipSwitch dip_switch;

/**
 * @brief Initialises pins
 */
void DipSwitch::init(void) {
    // Store ports and masks for fast digital read / write and initialize pins
    for (uint8_t col = 0U; col < sizeof(PINS_DIP_COL); ++col) {
        col_port_out_regs[col] = portOutputRegister(digitalPinToPort(pgm_read_byte(&(PINS_DIP_COL[col]))));
        col_masks[col] = digitalPinToBitMask(pgm_read_byte(&(PINS_DIP_COL[col])));
        pinMode(pgm_read_byte(&(PINS_DIP_COL[col])), OUTPUT);
        *(col_port_out_regs[col]) |= col_masks[col];
    }
    for (uint8_t row = 0U; row < sizeof(PINS_DIP_ROW); ++row) {
        row_port_in_regs[row] = portInputRegister(digitalPinToPort(pgm_read_byte(&(PINS_DIP_ROW[row]))));
        row_masks[row] = digitalPinToBitMask(pgm_read_byte(&(PINS_DIP_ROW[row])));
        pinMode(pgm_read_byte(&(PINS_DIP_ROW[row])), INPUT_PULLUP);
    }

    // Wait a bit for everything to settle
    delay(10);
}

/**
 * @brief Reads states of all dip switches by sweeping across columns and reading row values.
 * Result will be written into `states` variable. Most significant bit (0b10000000) is 1st dip switch.
 * 1 means ON, 0 means OFF
 */
void DipSwitch::read(void) {
    for (uint8_t col = 0U; col < sizeof(PINS_DIP_COL); ++col) {
        // Set target column to LOW
        *(col_port_out_regs[col]) &= ~col_masks[col];

        // Read rows and set bits in states variable
        for (uint8_t row = 0U; row < sizeof(PINS_DIP_ROW); ++row) {
            uint8_t state_mask = 1U << (7U - (row * sizeof(PINS_DIP_COL) + col));
            if (*(row_port_in_regs[row]) & row_masks[row])
                states &= ~state_mask;
            else
                states |= state_mask;
        }

        // Set column back to HIGH
        *(col_port_out_regs[col]) |= col_masks[col];
    }
}
