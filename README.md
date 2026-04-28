# Motor_Driver_Board
PCB for A4988 Stepper Driver + Arduino Pico

Arduino Pico pins used:
    40 - VDD
    1 - DIR
    2 - STEP
    4 - MS3
    5 - MS2
    6 - MS1


A4988 specs
- Output capacity: 35V, +-2A
- Operating voltage: 8-35V
- Continuous current per phase: 1A
- Max current per phase: 2A
- Logic voltage: 3-5.5V

A4988 Step Control
    Values in order of: MS1, MS2, MS3
        LO, LO, LO: Full step
        HI, LO, LO: 1/2 step
        LO, HI, LO: 1/4 step
        HI, HI, LO: 1/8 step
        HI, HI, HI: 1/16 step

