# Arduino-condenser_sprinkler
For the vehicle's AC condenser, assist in cooling.

Will only activate the water pump if the AC compressor is currently activated, even if overriden

## ***Functionalities***
  - Has padding delay before activating the pump, to ensure that the condenser fan was running before spraying,
    If the condenser was running longer than the threshold, will wait for next cycle instead, so the cooldowns
    will not be wasted
  - Override button to ignore cooldowns and delays

## ***Setup***
The following are the Arduino pin assignments:

 - 13 -> Can be connected to external LED, otherwise use the builtin LED
 - 3 -> External LED, indicator if the AC compressor was activated
 - 4 -> Hook to car AC compressor, ***WARNING***: use a protection circuit as this is inductive load with more than 12V
 - 7 -> Hook to external switch that pulls the signal to ground (used for override)
 - 6 -> Hook to the water pump driver for activation, ***WARNING***: use a driver circuit to drive the water pump
