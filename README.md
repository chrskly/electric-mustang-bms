# Electric Mustang BMS

This repo contains the hardware design and software for the custom BMS for my EV conversion project.

## Overview

The BMS is always on.

The battery is made up of two BMW PHEV battery packs.

Each battery pack has a positive and negative contactor which are activated by either the ignition key or the charger.

The BMS is able to inhibit the closing of the contactors in each battery pack individually. It only does this when the contactors are already open.

The car will also have precharge and main contactors in the HVJB. These are controlled by the inverter.

## Compiling/Installing

### Building the BMS code

```
cd src/build
cmake ../
make
```

### Building the test framework code

```
cd test/build
cmake ../
make
```

## State machine

### States

* standby
* drive
* batteryHeating
* charging
* batteryEmpty
* overTempFault
* illegalStateTransitionFault

### Events

* E_TEMPERATURE_UPDATE  - the readings from the battery temperature sensors have
                          updated values.
* E_CELL_VOLTAGE_UPDATE - the readings from the battery voltage sensors have
                          updated values.
* E_IGNITION_ON         - the ignition has been turned on.
* E_IGNITION_OFF        - the ignition has been turned off.
* E_CHARGING_INITIATED  - charging has been initiated.
* E_CHARGING_TERMINATED - charging has been stopped.

## Functionality

1. Provide protection for issues relating to paralleling packs. Deny contactor
   close when pack voltages (not cell voltages) differ by more than a certain
   number of mV. Instead, when going into drive mode, only close the
   contactors for the pack with the highest voltage and, when going into charge
   mode, only close the contactors for the pack with the lowest voltage. Car
   will still be drivable and full battery capacity will still be usable (by
   turning ignition off and on again).
2. When driving on a subset of packs, allow contactors to close when the pack
   voltages get back into alignment.
3. When charging on a subset of packs, allow contactors to close when the pack
   voltages get back into alignment.
4. Send CAN message to warn when cell(s) near overvolt.
5. Deny charging when cell(s) overvolt.
6. Warn when cell(s) near undervolt.
7. Deny drive when cell(s) undervolt.
8. When cells are below 0 degrees c, turn on heaters and deny charge until warm
   enough.
9. Deny drive when temperature over XX degress c.
10. Report range estimate.
11. Provide max charging rate value to charger.
12. Reset ISA shunt counter when battery has been charged to 100%.

## Connections

1. CAN port connected to main CAN bus.
2. CAN port connected to front battery CAN bus.
3. CAN port connected to rear battery CAN bus.
4. Low side switch for front battery contactors. Contactors are actually
   controlled by inverter, but BMS can override.
5. Low side switch for rear battery contactors. Contactors are actually
   controlled by inverter, but BMS can override.
6. Low side switch to turn on battery heater.
7. IGNITION_ON input signal
8. CHARGE_ENABLE input signal
9. CHARGE_INHIBIT output signal
10. DRIVE_INHIBIT output signal

## Pinout

```
|                                               |
| A7 A8 A9 A10 A11 A12     B1  B2  B3  B4 B5 B6 |
| A6 A5 A4 A3  A2  A1      B12 B11 B10 B9 B8 B7 |
+-----------------------------------------------+
```

### BMS

- A1  : GPIO_13 : IN_3  / BATT1_CONTACTOR_FEEDBACK
- A2  : GPIO_11 : IN_1  / POS_CONTACTOR_FEEDBACK
- A3  : GPIO_9  : IN_X  / CHARGE_ENABLE_IN
- A4  : GPIO_6  : OUT_3 / DRIVE_INHIBIT
- A5  : GPIO_4  : OUT_1 / CHARGE_INHIBIT
- A6  : GPIO_3  : OUT_X / BATT_2_INHIBIT
- A7  : GPIO_2  : OUT_X / BATT_1_INHIBIT
- A8  : GPIO_5  : OUT_2 / HEATER_ENABLE
- A9  : GPIO_7  : OUT_4
- A10 : GPIO_10 : IN_X  / IGNITION_ON_IN
- A11 : GPIO_12 : IN_2  / NEG_CONTACTOR_FEEDBACK
- A12 : GPIO_14 : IN_4  / BATT2_CONTACTOR_FEEDBACK


- B1 : BATT_2_CAN_H
- B2 : BATT_1_CAN_H
- B3 : MAIN_CAN_H
- B4 : GND
- B5 : 12V
- B6 : GND
- B7 : 5V
- B8 : 5V
- B9 : GND
- B10 : MAIN_CAN_L
- B11 : BATT_1_CAN_L
- B12 : BATT_2_CAN_L

### Tester

- A1  : GPIO_13 : IN_3  / DRIVE_INHIBIT
- A2  : GPIO_11 : IN_1  / CHARGE_INHIBIT
- A3  : GPIO_9  : IN_X  / HEATER_ENABLE
- A4  : GPIO_6  : OUT_3 / 
- A5  : GPIO_4  : OUT_1 / CHARGE_ENABLE
- A6  : GPIO_3  : OUT_X / 
- A7  : GPIO_2  : OUT_X / IGNITION_ON
- A8  : GPIO_5  : OUT_2 / 
- A9  : GPIO_7  : OUT_4 /
- A10 : GPIO_10 : IN_X  / 
- A11 : GPIO_12 : IN_2  / BATT_2_INHIBIT
- A12 : GPIO_14 : IN_4  / BATT_1_INHIBIT


- B1 : BATT_2_CAN_H
- B2 : BATT_1_CAN_H
- B3 : MAIN_CAN_H
- B4 : GND
- B5 : 12V
- B6 : GND
- B7 : 5V
- B8 : 5V
- B9 : GND
- B10 : MAIN_CAN_L
- B11 : BATT_1_CAN_L
- B12 : BATT_2_CAN_L

## CAN messages consumed by BMS

1. kWh, Ah, current, voltage data from ISA shunt to calculate SoC.

## CAN messages produced by BMS

1. Computed value of max charge or discharge current (or power) available from
   pack.
2. Overcurrent warning message.
3. Over/under voltage warning/alarm.
4. Over/under temp warning/alarm.
5. Cell delta voltage warning/alarm.
6. Max/min pack voltage.
7. SoC

## Configurables

1. minCellVoltage - disallow drive when cell voltage at or below this level.
2. maxCellVoltage - disallow charge when cell voltage at or above this level.
3. maxPackVoltageDelta - max allowed difference in voltage between packs. Above
   this, contactors will not be allowed to close.
4. minChargeTemp - disallow charging at or below this temperature.

## Charging

When in charge mode the BMS will emit a maximum allowed charge current in the
0x351 CAN message. The chargers will listen for this and adjust their request
accordingly.

There is a minimum temperature below which we must not charge. So we disallow
charging at or below -10°C.

Above the minimum, we need to limit the charge current. So we ramp up the
allowed current as temperature increases. This is between -10°C and 15°C.

In the 'happy temperature' range (between 15°C and 35°C) we allow the full 125A
(delivered by a 50kw charger). However, if the temperature of the pack increases
by more than 1°C per minute, derate the charge current by 10% for every degree
over.

Above 35°C, we ramp back down the current. We also derate based on temperature
increase on top of that.

It looks something like this.

```
current
   ^
   |
   |                    /----------------------\
   |                  /                         \
   |                /                            \
   |              /                               \
   |            /                                  \
   |          /                                     \
   |        /                                        \
   |      /                                           \
   |    /                                              \
   |  /                                                 \
   +-+------------------+---------------------+----------+----> temperature
    -10°               15°C                  35°C       40°C
```

## Welded Contactor Detection

There are four inputs on the BMS for welded contactor detection.

1. HVJB positive contactor
2. HVJB negative contactor
3. Battery 1 contactors
4. Battery 2 contactors

There are four bits in the status CAN message (0x352) for reporting
welded contactors (one bit for each of the above).

The weld detection only runs when the car is in standby mode (the
only time it makes sense).

Both the positive and negative contactos in the HVJB have dedicated
feedback inputs. The switched side of the auxiliary side of the
contactor should be fed into this input. I.e., feed it 12V when the
contactor is closed.

The battery contactors just have one input each. We can use diodes to
report if either of the battery contactors (positive or negative) are
welded.

## To Do

- [x] Fetch SoC from shunt and store in memory
- [x] Broadcast BMS mode
- [x] CHARGE_ENABLE input
- [x] IGNITION_ON input
- [x] DRIVE_INHIBIT output
- [ ] Use other core for comms?
- [ ] Implement balancing
- [x] Implement watchdog
- [ ] On startup, properly detect the state we should start in and immediately switch to that state
- [x] Emulate SimpBMS output CAN messages
- [x] Reset ISA Shunt when battery charged to 100%
- [x] ISA shunt heartbeat
- [x] Warn/alarm flags
- [x] Warn when modules are missing
- [ ] Deal with scenario when one pack is full and one pack is empty
- [ ] Communication error bit(s)
- [x] enable/disable regen bit
- [x] inhibit function naming confusing
- [x] charge_inhibit_reason can msg
- [x] drive_inhibit_reason can msg
- [x] transmit module liveness over can   
- [x] welded contactor detection

## Credits

This project uses the [adamczykpiotr/pico-mcp2515](https://github.com/adamczykpiotr/pico-mcp2515) 
library for CAN communication.

The [Tom-evnut/BMWPhevBMS](https://github.com/Tom-evnut/BMWPhevBMS) project was
used as a reference for how to communicate with this battery pack.
