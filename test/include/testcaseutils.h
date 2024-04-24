
bool wait_for_50_percent_soc(Bms* bms, int timeout);
bool wait_for_drive_inhibit_to_activate();
bool wait_for_bms_state_to_change_to_batteryEmpty();
bool wait_for_charge_inhibit_to_activate();