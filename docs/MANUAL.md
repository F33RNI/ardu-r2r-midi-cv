# ardu-r2r-midi-cv (aka CMCEC) usage manual

## Modes

### DIP switch meaning

|                       **----------**                       |                       **----------**                       |                                        **----------**                                       |                                        **----------**                                       |
|:----------------------------------------------------------:|:----------------------------------------------------------:|:-------------------------------------------------------------------------------------------:|:-------------------------------------------------------------------------------------------:|
| Omnichannel mode<br>(receive from any channel)             | Merge gate &<br>trigger ports                              | Clock divider<br>MSB                                                                        | Clock divider<br>LSB                                                                        |
|                           ⬛<br>🔲                           |                           ⬛<br>🔲                           |                                                                                      ⬛<br>🔲 | ⬛<br>🔲                                                                                      |
| 1st MIDI ch. -> 1st CV<br>2nd MIDI ch. -> 2nd CV           | 1st CV -> 1st G/T<br>2nd CV -> 2nd G/T                     | See table below<br>for more info                                                            | See table below<br>for more info                                                            |
|                       **----------**                       |                       **----------**                       |                                        **----------**                                       |                                        **----------**                                       |
| Enable arpeggiator<br>on 1st channel                       | Enable arpeggiator<br>on 2nd channel                       | Arpeggiator 1 direction<br>(ON = UP, OFF = DOWN)<br>Or polyphonic mode<br>(both must be ON) | Arpeggiator 2 direction<br>(ON = UP, OFF = DOWN)<br>Or polyphonic mode<br>(both must be ON) |
|                                                     ⬛<br>🔲 | ⬛<br>🔲                                                     |                                                                                      ⬛<br>🔲 | ⬛<br>🔲                                                                                      |
| Normal mode (or polyphonic<br>mode, both arps must be OFF) | Normal mode (or polyphonic<br>mode, both arps must be OFF) |                                                                                             |                                                                                             |
|                       **----------**                       |                       **----------**                       |                                        **----------**                                       |                                        **----------**                                       |

### Clock divider

| DIP  |   Value    |
|------|:----------:|
| ⬇️🔼 |  1/8 note  |
| 🔼⬇️ |  1/4 note  |
| 🔼🔼 |  1/2 note  |
| ⬇️⬇️ | Whole note |

### 🚧 Manual in progress... 🚧
