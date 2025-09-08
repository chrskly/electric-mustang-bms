
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

# List of Test Cases

* 001 : Ensure car cannot be driven when battery is empty, starting in STANDBY
  state.
* 002 : Ensure car cannot be driven when battery is empty, starting in DRIVE
  state.
* 003 : When battery is empty and overheating, but the temperature drops, the
  car should still not be drivable as the battery is still empty.
* 004 : When battery is empty and overheating, but the SoC rises the car should
  still not be drivable as the battery is still too hot.
* 005 : When battery is empty and we're charging, but the charging stops, the
  car should still not be drivable as the battery is still empty.
* 006 : When battery is full, the car should not be allowed to charge. Start in
  STANDBY state.
* 007 : When battery is full, the car should not be allowed to charge. Start in
  DRIVE state.
* 008 : When battery is full, the car should not be allowed to charge. Start in
  CHARGE state.
* 101 : When in standby state, and the pack voltages differ, the BMS should
  inhibit the battery contactor close.
* 102 : When in drive state, and the pack voltages differ, the BMS should not
  inhibit the battery contactor close. I.e., don't open the contactors when
  driving.
* 103 : When packs are imbalanced, and we go into drive mode from standby, only
  high pack should be enabled. Low pack should remain inhibited.
* 104 : When packs are imbalanced, and we go into standby mode from drive mode,
  both packs should be inhibited.
* 105 : When packs are imbalanced, and we go into charging mode from standby
  mode, only the low pack should be enabled. High pack should remain inhibited.
* 106 : When packs are imbalanced, and we stop charging, both packs should be
  inhibited.
* 107 : When we are charging on imbalanced packs, and the voltages equalise,
  both packs should be uninhibited.
* 108 : When we are driving on imbalanced packs, and the voltages equalise, both
  packs should be uninhibited.
* 109 : When we are driving on imbalanced packs, and we start charging, the BMS
  should inhibit the charging and driving and go into an error state (illegal
  state transition fault).
* 110 : When in standby state, with imabalanced packs, and the voltages
  equalise, the BMS should disable the contactor inhibition.
* 111 : When charging, and packs go into imbalanced state, the BMS should not
  inhibit contactor close. I.e., don't open the contactors when charging.
* 201 : When the battery is too cold to charge, the BMS should inhibit charging.
  Start in STANDBY state.
* 202 : When the battery is too cold to charge, the BMS should inhibit charging.
  Start in DRIVE state.
* 203 : When charge initiated, and the battery is too cold, heater should
  activate and charging should continue to be inhibited.
* 204 : Charging has been initiated, but battery is too cold so heater is on.
  When temperature of battery goes over 0°C, disable battery heating.
* 205 : If we're charging and some sensors are reading too cold to charge, but
  some sensors are reading too hot, then heater should not be enabled.
* 206 : When charging, and the battery gets too hot, charging should be
  inhibited.
