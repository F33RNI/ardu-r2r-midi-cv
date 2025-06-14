/**
 * @file utils.h
 * @author Fern Lane
 * @brief Useful functions
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

#ifndef UTILS_H__
#define UTILS_H__

#include <Arduino.h>

/**
 * @brief Arduino's map() function but for float
 */
inline float map_f(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
 * @brief Arduino's map() function but for uint8_t
 */
inline uint8_t map_uint8(uint8_t x, uint8_t in_min, uint8_t in_max, uint8_t out_min, uint8_t out_max) {
    return static_cast<uint8_t>((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

/**
 * @brief Arduino's map() function but for uint16_t
 */
inline uint16_t map_uint16(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
    return static_cast<uint16_t>((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

/**
 * @brief Converts MIDI note into frequency
 * @param cents MIDI note number in cents (e.g., 6000 = C4)
 * @return float frequency in Hz
 */
inline float note_to_hz(uint16_t cents) {
    return 440.0f * powf(2.0f, static_cast<float>(static_cast<int16_t>(cents) - 6900) / 1200.0f);
}

/**
 * @brief Calculates deviation between 2 frequencies in cents
 * @return int16_t deviation in cents or INT16_MIN/INT16_MAX on error
 */
inline int16_t hz_to_cents_deviation(float target, float measured) {
    if (target <= 0.0f)
        return INT16_MAX;
    if (measured <= 0.0f)
        return INT16_MIN;

    float cents = 1200.0f * (logf(measured / target) / logf(2.0f));
    if (cents >= static_cast<float>(INT16_MAX))
        return INT16_MAX;
    if (cents <= static_cast<float>(INT16_MIN))
        return INT16_MIN;
    return static_cast<int16_t>(roundf(cents));
}

/**
 * @brief Converts MIDI note into mV in 1V/Oct scale. Ex: 6000 (C4) = 4000mV
 */
inline float note_to_mv(uint16_t cents) {
    if (cents <= 1200U)
        return 0.0f;

    uint8_t octave = static_cast<uint8_t>(cents / 1200U);
    float note = static_cast<float>(cents % 1200U) / 1200.0f;
    return 1000.0f * (static_cast<float>(octave - 1U) + note);
}

/**
 * @brief Calculates the MIDI note in cents from frequency
 * @return uint16_t MIDI note in cents (non-clamped)
 */
inline uint16_t hz_to_note(float freq) {
    if (freq <= 0.0f)
        return 0U;
    return static_cast<uint16_t>(roundf(6900.0f + 1200.0f * (logf(freq / 440.0f) / logf(2.0f))));
}

#endif
