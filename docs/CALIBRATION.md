# ardu-r2r-midi-cv (aka CMCEC) calibration and tuner manual

## Entering calibration mode

To enter calibration mode, press and hold calibration button (see `PIN_CALIB_BTN` in `include/pins.h` file,
default pin: `12`) and power on / reset module. It should start blinking 1st LED with blue after loading.
If failed, please try again.

## Calibration modes

Currently there are 7 calibration modes:

1. 🔵⚫ **1st DAC gain calibration** - semi-manual 1st channel DAC gain calibration
2. ⚫🔵 **2nd DAC gain calibration** - semi-manual 2nd channel DAC gain calibration
3. 🟢🟢 **Tuner** - simple tuner (uses linearity calibration)
4. 🟡⚫ **1st VCO linearity calibration** - automatically calibrates 1st channel linearity
5. ⚫🟡 **2nd VCO linearity calibration** - automatically calibrates 2nd channel linearity
6. 🔴⚫ **1st VCO linearity reset** - resets stored linearity calibration of 1st channel
7. ⚫🔴 **2nd VCO linearity reset** - resets stored linearity calibration of 2nd channel

You can cycle trough them with short presses on button. Each mode will blink with 500ms period with listed colors.

To enter each calibration mode, use long button press (> 1s)

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

It's possible to use this module as tuner for VCO. Octave and note selects with DIP switch and module will show pitch
deviation using both LEDs.

1. Enter **Tuner mode** by long pressing on a button
2. Connect VCO's output (preferably, square wave) into CLOCK IO / VCO input (see `PIN_CALIB_VCO` in `include/pins.h`
  file).
  *In order to protect Arduino from high VCO output, make sure, resistor and diodes to power rails are connected*
3. Select target octave and note using DIP switch (see table below)
4. Adjust VCO's frequency to "center" frequency. (Eg. 1st LED (🟠⚫) will be brighter and more red-ish the lower the
  actual frequency is). Below / above 250 cents, the LED will be fully red

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
> **NOTE:** Maximum possible range is C0 (12) - G9 (127) (not considering DAC output and VCO's limitations).
> Notes outside this range will be clamped to 12-127 range

In order to exit tuner mode, just long press button again.

## 🟨 VCO linearity calibration

One of the main features of this module is automatic linearity calibration based on VCO's output (closed loop).
This is done by sweeping across an entire output range and measuring VCO's frequency.
This produces calibration matrix that is stored in the EEPROM and will be loaded at the next boot.
This matrix is used in normal mode as well as in **Tuner mode**.

Internally, **VCO linearity calibration** consist of 3 modes:

1. Tuner - helps you to set middle calibration point. It will wait for you to tune VCO, then wait 5 more seconds
  (see `CALIB_VCO_START_DELAY_INIT` in `include/calibration.h` file) before switching to the 2nd stage
2. Lowest note detection - gradually decreases output to 10% of minimum and determines lowest note
3. Linearity calibration - gradually sweeps from lowest point to 90% of maximum of output and records all
  notes on the way

## 🟥 VCO linearity reset

If you need to remove saved linearity calibration in order to get direct note -> CV conversion, it's possible to reset calibration of any channel. After this, MIDI note number will be converted into target voltage directly and just multiplied on gain (with offset calibration) and compensated for supply voltage before sending it to the DAC.

For that, just long press the button while LED is blinking on desired channel. This will reset saved calibration matrix and switch to the next mode (or next channel).
