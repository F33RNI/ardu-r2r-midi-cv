# 🎛️ ardu-r2r-midi-cv (aka CMCEC) Calibration & Tuner guide

## 🔧 Entering calibration mode

To enter calibration mode:

1. **Press and hold** the calibration button (defined as `PIN_CALIB_BTN` in `include/pins.h`, default pin: `12`)
2. While holding the button, **power on** or **reset** the module
3. The **1st LED** should start blinking **blue** after boot

> If it doesn't work the first time, don't worry and pls try again

## Calibration modes

There are **7 calibration modes**, each represented by a unique LED blink pattern:

| # | LED Pattern  | Mode Description                       |
| - | ------------ | -------------------------------------- |
| 1 |     🔵⚫     | 1st DAC Gain Calibration (semi-manual) |
| 2 |     ⚫🔵     | 2nd DAC Gain Calibration (semi-manual) |
| 3 |     🟢🟢     | Tuner Mode                             |
| 4 |     🟡⚫     | 1st VCO Linearity Calibration          |
| 5 |     ⚫🟡     | 2nd VCO Linearity Calibration          |
| 6 |     🔴⚫     | Reset 1st VCO Linearity                |
| 7 |     ⚫🔴     | Reset 2nd VCO Linearity                |

- **Short press** - cycle through modes
- **Long press** (hold for >1 second) - enter the selected mode

> Each mode blinks with a 500ms interval to help you recognize it

## 🟦 DAC gain calibration

Due to error in internal 1.1V reference, op-amp resistors and other factors, you need to calibrate actual DAC gain.

In this mode, CV output of selected channel will be set to `3000mV` (see `CALIB_DAC_TARGET_1` and `CALIB_DAC_TARGET_2`
in `include/calibration.h` file) and you need to offset gain using DIP switch.

Base gain is `1.75` by default (see `GAIN_1_BASE` and `GAIN_2_BASE` in `include/dac.h` file).
You can offset this value using DIP switch to `+/- 0.127` (`1.623` - `1.877`).

Top left switch selects direction of change. in ON (up) position it's positive offset,
in OFF (down) it's negative offset.

> **NOTE:** Positive offset means output voltage will be decreased

Other 7 switches represents 127 in binary with most significant bit being 2nd top.

So, the procedure is as follows:

1. Enter **DAC gain calibration mode** by long pressing on a button
2. Connect precise multimeter to the output of CV of selected channel
3. If it's ABOVE 3V, set 1st top DIP switch to ON (up) position, if it's BELOW 3V, set 1st top DIP switch to OFF (down)
  position
4. Adjust other 7 switches to make output as close to 3V as possible
5. When calibrated, long press button again. This will writes calibration data into EEPROM. Next mode should start
  blinking after that. Repeat for both channels

> **Example configuration**
>
> Multimeter measures `3.026V`. So, real gain is `(3.026 / 3) * 1.75 = ~1.765`. That gives offset of `+0.015`
> that must be added to the `1.75`. So, by setting 1st DIP switch to ON (up) position and 4 lower bits also
> to ON (up) (1000 / 1111, *1111 is 15 in binary*) output voltage should drop to exactly `3V`

## 🟩 Tuner

This module can also act as a **simple tuner** for your VCO.

### How it works

- You choose the **target note** using DIP switches
- The module shows how close the VCO frequency is to the target using the **LEDs**

1. Enter **Tuner Mode** with a long button press
2. Connect your VCO's **square wave output** to the **CLOCK IO / VCO input** (see `PIN_CALIB_VCO` in `include/pins.h`)
  *In order to protect Arduino from high VCO output, make sure, resistor and diodes to power rails are connected*
3. Use the DIP switches to set **octave and note**
4. Adjust VCO's frequency to "center" frequency. (Eg. 1st LED (🟠⚫) will be brighter and more red-ish the lower the
  actual frequency is). Below / above 250 cents, the LED will be fully red. When tuned, both LEDs will be slightly green

| Octave | Top 4 DIP switches | Note | Bottom 4 DIP switches |
|--------|--------------------|------|-----------------------|
|    0   |      ⬇️⬇️⬇️⬇️      |   C  |        ⬇️⬇️⬇️⬇️       |
|    1   |      ⬇️⬇️⬇️🔼      |  C#  |        ⬇️⬇️⬇️🔼       |
|    2   |      ⬇️⬇️🔼⬇️      |   D  |        ⬇️⬇️🔼⬇️       |
|    3   |      ⬇️⬇️🔼🔼      |  D#  |        ⬇️⬇️🔼🔼       |
|    4   |      ⬇️🔼⬇️⬇️      |   E  |        ⬇️🔼⬇️⬇️       |
|    5   |      ⬇️🔼⬇️🔼      |   F  |        ⬇️🔼⬇️🔼       |
|    6   |      ⬇️🔼🔼⬇️      |  F#  |        ⬇️🔼🔼⬇️       |
|    7   |      ⬇️🔼🔼🔼      |   G  |        ⬇️🔼🔼🔼       |
|    8   |      🔼⬇️⬇️⬇️      |  G#  |        🔼⬇️⬇️⬇️       |
|        |                    |   A  |        🔼⬇️⬇️🔼       |
|        |                    |  A#  |        🔼⬇️🔼⬇️       |
|        |                    |   B  |        🔼⬇️🔼🔼       |

> Ex.: **C4** - ⬇️🔼⬇️⬇️ ⬇️⬇️⬇️⬇️ (0100 / 0000) - 4000mV
>
> Ex.: **A4** - ⬇️🔼⬇️⬇️ 🔼⬇️⬇️🔼 (0100 / 1001) - 4750mV
>
> 🎵 Supported range: C0 (note 12) to G9 (note 127). Values outside this range are clamped

To exit tuner mode, just long-press the button again.

## 🟨 VCO Linearity Calibration (Modes 4 & 5)

One of the main features of this module is automatic linearity calibration based on VCO's output (closed loop).
This is done by sweeping across an entire output range and measuring VCO's frequency.
This produces calibration matrix that is stored in the EEPROM and will be loaded at the next boot.
This matrix is used in normal mode as well as in the **Tuner mode**.

Internally, **VCO linearity calibration** consist of 3 modes:

1. Tuner - Helps you to set middle calibration point. It will wait for you to tune VCOю After tuning,
  it waits 5 seconds before moving on (see `CALIB_VCO_START_DELAY_INIT`)
2. Drop to lowest voltage - Decreases output to `CALIB_VCO_MV_MIN` and waits 2 seconds for VCO's frequency to stabilize
3. Linearity calibration - Gradually sweeps from lowest point to 95% of maximum of output voltage and records all
  notes on the way

### 🟥 Reset VCO Linearity Calibration (Modes 6 & 7)

If you need to remove saved linearity calibration in order to get direct note -> CV conversion, it's possible to reset calibration of any channel. This disables calibration-based corrections, and the module will use **raw note-to-voltage** conversion (with gain/offset compensation).

To reset:

1. Select the appropriate channel to reset (🔴⚫ or ⚫🔴)
2. Long-press the button
3. The calibration matrix is erased, and the module moves to the next calibration mode (7 or 1)
