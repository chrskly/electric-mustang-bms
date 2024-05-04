
# Testing framework for BMS

## Connections between BMS and test harness

* Main CAN bus
* Battery 1 CAN bus
* Battery 2 CAN bus
* CHARGE_ENABLE, input, 12V when active
* IGNITION_ENABLE, input, 12V when active
* Battery 1 inhibit, output, GND when active
* Battery 2 inhibit, output, GND when active
* CHARGE_INHIBIT, output, GND when active
* HEATER_ENABLE, output, GND when active


--------------------------------------------------------------------------------


## Test Cases : [0xx] : Battery empty/full


### Test Case 001

Description : Ensure car cannot be driven when battery is empty

Pre-conditions
* ignition on
* charge off
* batt1 inhibit off
* batt2 inhibit off
* chg inhibit off
* heater enable off

Event : Any cell is empty

Result : DRIVE_INHIBIT signal activates


### Test Case 002

Description : Ensure battery cannot be charged when battery is full

Pre-conditions
* ignition off
* charge off
* batt1 inhibit off
* batt2 inhibit off
* chg inhibit off
* heater enable off

Event : Any cell is full

Result : CHARGE_INHIBIT signal activates


--------------------------------------------------------------------------------


## Test Cases : [1xx] : Pack-level voltage safety


### Test Case 101

Description : Inhibit battery contactor close when pack voltages differ

Pre-conditions
* ignition off
* charge off
* batt1 inhibit off
* batt2 inhibit off
* charge inhibit off
* heater enable off

Event : Voltage difference between packs is greater than SAFE_VOLTAGE_DELTA_BETWEEN_PACKS volts.

Result : Both batt1 inhibit and batt2 inhibit signals activate


### Test Case 102

Description : Do not inhibit battery contactor close when pack voltage differ and ignition is on.

Pre-conditions
* ignition on
* charge off
* batt1 inhibit off
* batt2 inhibit off
* charge inhibit off
* heater enable off

Event : Voltage difference between packs is greater than SAFE_VOLTAGE_DELTA_BETWEEN_PACKS volts.

Result : batt1 inhibit and batt2 inhibit remain disabled.


### Test Case 103

Description : Ignition turned on when battery contactors are inhibited.

Pre-conditions
* ignition off
* charge off
* batt1 inhibit on
* batt2 inhibit on
* charge inhibit off
* heater enable off

Event : Voltage of batt 1 higher than batt 2. Ignition turned on.

Result : Batt 1 inhibit deactivates


### Test Case xx

Description : Ignition turned off when battery contactors are inhibited.

Pre-conditions
* ignition on
* charge off
* batt1 inhibit off
* batt2 inhibit on
* charge inhibit off
* heater enable off

Event : Ignition turned off.

Result : Batt 1 inhibit activates.


### Test Case xx

Description : Start charging when battery contactors are inhibited.

Pre-conditions
* ignition off
* charge off
* batt1 inhibit on
* batt2 inhibit on
* charge inhibit off
* heater enable off

Event : Voltage of batt 1 higher than batt 2. Charging starts (CHARGE_ENABLE input signal activates).

Result : Batt 2 inhibit deactivates.


### Test Case xx

Description : Stop charging when battery contactors are inhibited.

Pre-conditions
* ignition off
* charge on
* batt1 inhibit on
* batt2 inhibit off
* charge inhibit off
* heater enable off

Event : Voltage of batt 1 higher than batt 2. Charging ceases.

Result : Batt 2 inhibit activates.


### Test Case xx

Description : Charging on one pack, voltage equalises

Pre-condition
* ignition off
* charge on
* batt1 inhibit off
* batt2 inhibit on
* charge inhibit off
* heater enable off

Event : Voltage of batt1 == voltage of batt2.

Result : batt2 inhibit deactivates.


### Test Case xx

Description : Driving on one pack, voltage equalises

Pre-condition
* ignition on
* charge off
* batt1 inhibit off
* batt2 inhibit on
* charge inhibit off
* heater enable off

Event : Voltage of batt1 == voltage of batt2.

Result : batt2 inhibit deactivates.


### Test Case xx

Description : Driving on one pack, begin charging while ignition still on.

Pre-condition
* ignition on
* charge off
* batt1 inhibit off
* batt2 inhibit off
* charge inhibit off
* heater enable off

Event : CHARGE_ENABLE signal activates.

Result : DRIVE_INHIBIT and CHARGE_INHIBIT signals should activate.


--------------------------------------------------------------------------------


## Test Cases : [2xx] : Temperature


### Test Case xx

Description : Battery too cold to charge.

Pre-conditions
* ignition off
* charge off
* batt1 inhibit off
* batt2 inhibit off
* charge inhibit off
* heater enable off

Event : Any battery temperature sensor drops below 0c

Result : CHARGE_INHIBIT output signal activates


### Test Case xx

Description : Battery warm enough to charge again.

Pre-conditions
* igition off
* charge off
* batt1 inhibit off
* batt2 inhibit off
* chg inhibit on
* heater enable off

Event : All battery sensor temperatures above 0c

Result : CHARGE_INHIBIT output signal deactivates


### Test Case xx

Description : Too cold to charge, but charge requested.

Pre-condition
* ignition off
* charge off
* batt1 inhibit off
* batt2 inhibit off
* charge inhibit on (due to low battery sensor temp)
* heater enable off

Event : CHARGE_ENABLE input signal activates

Result : HEATER_ENABLE output signal activates


### Test Case xx

Description : Battery too hot to charge.

Pre-condition
* ignition off
* charge off
* batt1 inhibit off
* batt2 inhibit off
* charge inhibit off
* heater enable off

Event : any temperature sensor is over x degrees.

Result : CHARGE_INHIBIT output signal activates.


### Test Case xx

Description : Limit charging current when battery partially overheats.

Pre-condition
* ignition off
* charge on
* batt1 inhibit off
* batt2 inhibit off
* charge inhibit off
* heater enable off

Event : Set one of the battery sensors between CHARGE_THROTTLE_TEMP_LOW and CHARGE_THROTTLE_TEMP_HIGH.

Result : Max charge current in 0x351 message should be scaled based on temperature.


### Test Case xx

Description : 