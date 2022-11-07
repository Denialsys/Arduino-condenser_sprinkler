/*
 * Author: Jenver I.
 * 
 * Sketch for controlling the water pump that sprays water to the
 * vehicle AC condenser, which should assist in rapidly cooling the
 * condensed coolant
 * 
 * Used home appliance automation
 * 
 * This will have the functions to:
 *    - Automatically spray after a set cooldown
 *    - Override the cooldown and spray on a button press
 *    - Will only spray water while the compressor was ON
 */

#include <EEPROM.h>

bool isPumpEnabled = true;
#define pulseWidth 2200   // length of time the water pump was on
#define pulsePadding 2000 // delay before switching on the water pump when the compressor was on

// If compressor was ON longer than this threshold when cooled down
// The water pump cycle should not commence since the compressor may turn off before the cycle completes
#define thresholdWindow 2000
#define pumpDelay 90000 // Delay before next water pump cycle
bool isCycleOnGoing = false;
short cyclePhase = 0;
unsigned long cycleStart = 0;

// short overrideMode = 0; // Future implem: for changing the cycle cool down

struct instrumentState
{
  unsigned long lastOn = 0;
  unsigned long initialOn = 0;
  bool isOn = false;
  bool lastState = false;
  short pin;
  unsigned long triggerTime = 0; // For debouncing
};

instrumentState waterPump;
instrumentState acCompressor;

#define assistStateOut 13 // Indicator pin for the assist
#define compressorOut 3   // Indicator pin for the compressor state
#define compressorRead 4  // Pin for reading the state of the compressor
#define pulseRead 7    // Pin for pulsing the water pump
#define waterPumpOut 6 // Pin for the setting the water pump on or off
#define debounceTime 150 // Debounce time in milliseconds
#define PUMPSTATEADDRESS 5 // Address in the eeprom to read

// #define serialPrintDelay 700 // Delay before printing out again
// unsigned long serialTiming = 0;

void setup()
{

  Serial.begin(9600); // DELETE

  // put your setup code here, to run once:
  pinMode(assistStateOut, OUTPUT);
  pinMode(compressorOut, OUTPUT);
  pinMode(compressorRead, INPUT_PULLUP);
  pinMode(pulseRead, INPUT_PULLUP); // Pin for pulsing/flushing
  pinMode(waterPumpOut, OUTPUT);
  waterPump.pin = waterPumpOut;
  acCompressor.pin = compressorRead;
  Serial.println("Pins initialized"); // DELETE

  //get the last compressor state, incase the MCU was turned on while compressor was already running
  acCompressor.isOn = !digitalRead(compressorRead);
  acCompressor.lastState = !digitalRead(compressorRead);

  //  EEPROM.put(PUMPSTATEADDRESS, true);
  EEPROM.get(PUMPSTATEADDRESS, isPumpEnabled);
  Serial.println("Pins states initialized"); // DELETE

  // Indicate the state of the AC assist
  digitalWrite(assistStateOut, isPumpEnabled);
  if (!isPumpEnabled)
  {
    Serial.println("Assist disabled"); // DELETE
    return;
  }
}

void loop()
{

  // If enabled then enable the features
  digitalWrite(assistStateOut, isPumpEnabled);
  if (!isPumpEnabled) return;

  // Check first if the compressor was on
  // acCompressor.isOn = !digitalRead(acCompressor.pin );
  debounceButton(acCompressor);

  // Just get the compressor state and output it to pin
  digitalWrite(compressorOut, acCompressor.isOn);

  // If compressor is on, check if the pump needs to be turned on
  if (acCompressor.isOn)
  {

    // If currently being overridden ignore the following logics
    if (isPumpOverriden())
      return;

    // // Enable or disable the water pump
    if ((isWithinThreshold() && cyclePhase == 0) || (cyclePhase > 0))
    {

      switch (cyclePhase)
      {
      case 0: // Start of the cycle, previous cycle ended
        cycleStart = millis();
        cyclePhase = 1;
        break;
      case 1: // Start rendering padding
      case 2: // Padding render ongoing
        cyclePhase = renderPadding(cycleStart);
        break;
      case 3: // Start the water pump pulse
      case 4: // Water pump pulse on going
        cyclePhase = toggleWaterPump(cycleStart);
        break;
      case 5: // Start the cool down
      case 6: // Cool down on going
        cyclePhase = coolDown(cycleStart);
        break;
      }
    }
  }
  else
  {

    // If compressor is off, switch off the water pump too
    acCompressor.lastOn = millis();
    waterPump.isOn = LOW;
    waterPump.lastState = LOW;
    waterPump.lastOn = millis();

    // If the cycle was aborted prematurely, reset and start over again
    if (cyclePhase < 5) cyclePhase = 0;

  }
  digitalWrite(waterPump.pin, waterPump.isOn);
}

void debounceButton(struct instrumentState &button)
{
  button.isOn = !digitalRead(button.pin);
  if (!digitalRead(button.pin)){
    button.isOn = true;
    button.initialOn = millis();
  }else{
    button.isOn = false;
  }
  // // Debouncing causes irregularities
  // if (!digitalRead(button.pin))
  // { // The button was pulled up
  //   if ((!button.isOn) && (millis() - button.triggerTime > debounceTime))
  //   {
  //     button.isOn = true;
  //     button.initialOn = millis();
  //   }
  //   else
  //   {

  //     button.triggerTime = millis();
  //   }
  // }
  // else
  // {
  //   button.isOn = false;
  // }
}

// Main override, flush the water pump, do nothing if not overriden
bool isPumpOverriden()
{
  bool pulseStatus = !digitalRead(pulseRead);

  // Override ON
  if (pulseStatus)
  {
    
    waterPump.isOn = true;
    waterPump.initialOn = millis();
    cyclePhase = 6;                              // Go directly straight to cooldown phase
    digitalWrite(waterPump.pin, waterPump.isOn); // Overrride will skip the normal cycle so turn ON the pin here
  }
  else
  {
    // Override OFF
    if (cyclePhase == 6 || cyclePhase < 3)
    {
      waterPump.isOn = false;
    }
  }
  return pulseStatus;
}

// Check if the cycle can execute by checking if within the time window
bool isWithinThreshold()
{

  // In case the compressor has turned OFF and turned on again
  unsigned long timeDiff = millis() - acCompressor.initialOn;
  if (timeDiff < thresholdWindow)
  {

    return true;
  }
  // In case the compressor is ON longer than the water pump cycle
  else if (timeDiff > pulsePadding + pulseWidth + pumpDelay)
  {

    return true;
  }
  return false;
}

// Wait the time padding before enabling the water pump
short renderPadding(unsigned long cycleStart)
{
  short cyclePhase = 2;
  if (millis() - cycleStart >= pulsePadding)
  {

    cyclePhase = 3;
  }
  return cyclePhase;
}

// Check if the water pump needs to be turn on or off
short toggleWaterPump(unsigned long cycleStart)
{

  short cyclePhase = 4;
  if (millis() - cycleStart >= pulsePadding + pulseWidth)
  {

    waterPump.lastOn = millis();
    waterPump.isOn = LOW;
    cyclePhase = 5;

    // If the last turn on for the water pump exceeds the delay and the padding, turn it on
  }
  else
  {

    waterPump.initialOn = millis();
    waterPump.isOn = HIGH;
  }

  return cyclePhase;
}

// Cooldown period after activating the pump
short coolDown(unsigned long cycleStart)
{

  short cyclePhase = 6;
  if (millis() - cycleStart >= pulsePadding + pulseWidth + pumpDelay)
  {

    cyclePhase = 0;
  }
  else
  {

    waterPump.isOn = false;
  }

  return cyclePhase;
}

// // Serial print line with delay
// void timedPrint(String toPrint)
// {
//   if (millis() - serialTiming >= serialPrintDelay)
//   {
//     Serial.println(toPrint);
//     serialTiming = millis();
//   }
// }
