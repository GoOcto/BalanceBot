# BalanceBot

An Android App that communicates over USB to some hardware that drives the device around while balancing it on two wheels

### Work in Progress

The concept is relatively simple: the code in the Arduino compatible board keeps the bot in balance. The Android app reads gyros and accelerometers and keeps updating the Arduino with the data. ultimately the Android code could be expanded to do much more including telling the Arduino-based drive mechanism where to drive to.

1. Communication step is mostly complete.
2. Arduino code for balancing, needs to be merged into this project.

The hardware I am using includes:
- Pololu AStar32U4 Micro board (Arduino compatible) (Pololu part#3101)
- DRV8835 Dual Motor Driver Carrier (Pololu part#2135)
- Magnetic Encoder Pair Kit for Mini Plastic Gearmotors (Pololu part#1523)
- 2 120:1 Mini Plastic Gearmotor HP with extended motor shafts (to receive the encoders) (Pololu part#1520)
- Pololu Wheel 90×10mm Pair (Pololu part#1436)

I'm not yet 100% certain if the motors have an acceptable gear ratio with regards to the wheel diameters. To be determined.

Similar hardware ought to work. I will post more about the hardware setup after I do more testing.

