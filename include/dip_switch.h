/**
 * @file dip_switch.h
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

#ifndef DIP_SWITCH_H__
#define DIP_SWITCH_H__

#include <Arduino.h>

class DipSwitch {
  public:
    void init(void);
    void read(void);
    uint8_t states;

  private:
    volatile uint8_t *col_port_out_regs[4], *row_port_in_regs[2];
    uint8_t col_masks[4], row_masks[2];
};

extern DipSwitch dip_switch;

#endif
