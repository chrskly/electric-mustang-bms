# Functional Spec for BMS

## Overview

The BMS is always on.

The battery is made up of two BMW PHEV battery packs.

Each battery pack has a positive and negative contactor which are activated by either the ignition key or the charger.

The BMS is able to inhibit the closing of the contactors in each battery pack individually. It only does this when the contactors are already open.

The car will also have precharge and main contactors in the HVJB. These are controlled by the inverter.

## State machine

### States
* standby
* drive
* charging
* batteryEmpty
* fault

### Events
* E_TEMPERATURE_UPDATE  - the readings from the battery temperature sensors have
                          updated values.
* E_CELL_VOLTAGE_UPDATE - the readings from the battery voltage sensors have
                          updated values.
* E_IGNITION_ON         - the ignition has been turned on.
* E_IGNITION_OFF        - the ignition has been turned off.
* E_CHARGING_INITIATED  - charging has been initiated.
* E_CHARGING_TERMINATED - charging has been stopped.
* E_EMERGENCY_SHUTDOWN  - something bad has happened. Shut down as soon as possible.

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

## To Do

- [X] Fetch SoC from shunt and store in memory
- [ ] Broadcast BMS mode
- [x] CHARGE_ENABLE input
- [x] IGNITION_ON input
- [ ] DRIVE_INHIBIT output
- [ ] Use other core for comms?
- [ ] Implement balancing
- [ ] Implement watchdog
- [ ] On startup, properly detect the state we should start in and immediately switch to that state
- [X] Emulate SimpBMS output CAN messages
- [X] Reset ISA Shunt when battery charged to 100%
- [ ] ISA shunt heartbeat
- [ ] Warn/alarm flags

## Credits

This project uses the [adamczykpiotr/pico-mcp2515](https://github.com/adamczykpiotr/pico-mcp2515) 
library for CAN communication.

The [Tom-evnut/BMWPhevBMS](https://github.com/Tom-evnut/BMWPhevBMS) project was
used as a reference for how to communicate with this battery pack.
