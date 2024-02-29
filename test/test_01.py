#!/usr/bin/env python

from charger import Charger

def test_01():
    """
    ### Test Case 01

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
    """

    # Reset

    # Ensure ignition is on

    # Set a cell empty

