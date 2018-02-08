// Control code for the Tooth brushing exhibit
// By Cam Chalmers
// Cam@BlueLochiel.com
//
// We have a vibration sensor mounted on the back of the teeth
// We also have 5 heaters and two tempature sensors
// The heaters keep the teeth at a maintance tempature just below the point where the color will change
// When the vibration sensor see's that someone is brushing the teeth, the heaters heat up the teeth to just above the color change point


// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>

// Heater Constants
// Heaters are arrayed Left to Right. 1,2,3 on the top row. 4,5 on the bottom row 
// [0][1][2]
//  [3][4]
// These should be PMW pins. While we aren't using PMW, it's nice to have the option later
const int NumHeaterPins = 3;
const int HeaterPin[NumHeaterPins] = {9,10,11}; // pin 9 = heaters 0&3, 10=2&4, 11=1
const int StandByTarget = 25; //Temp to preheat the teeth too. Should not cause any color change
const int HeatTarget = 33; //Temp to heat to teeth too to change color.


// Vibration Sensor Trigger Constants
const int TriggerPin = 0;
const int TriggerThreshold = 100; // value from the vibration tigger that needs to be crossed (both up and down)
const int TriggerTimeframe = 3000; // Milliseconds that the threshold has to be crossed
const int TriggerFrequency = 2; // number of times the Threshold needs to be crossed (up and down) every Timeframe


//Temp Sensor Constants
//All temps are in C.
const int TempatureLeftPin = 2;
const int TempatureRightPin = 3; 
const int TempatureReadDelay = 1000; //Reading the sensors causes the ground to float, we space out temp reads to reduce that impact
OneWire LeftWire(TempatureLeftPin);
OneWire RightWire(TempatureRightPin);

DallasTemperature LeftSensors(&LeftWire);
DallasTemperature RightSensors(&RightWire);

DeviceAddress LeftTemp;
DeviceAddress RightTemp;

const int LowerBound = -20; //Temp's below this will be considered a faulty sensor
const int DeviationBound = 10; //Degrees C that the sensors can disagree on temp before they will be considered faulty
const int HeatOnDuration = 15000; // Once we turn on the heat, how long do we keep it on?
const int DebugDelay = 1000;

//Global Variables
bool SpikeUp = false; //Is the vibration sensor currently above the spike point?
unsigned long SpikeHistory[TriggerFrequency]; //Array to store the most recent spike events.
unsigned long LastDebugMsg = 0; //Time of the last Debug Msg
unsigned long HeatEnableTime = 0; //Time Heater was last turn on
unsigned long LastTempatureReadTime = 0; //Time of the last temp read

int TargetTempature = 0; //Current Temp Target. We change this to turn the heaters on or off
bool HeaterEnable[NumHeaterPins]; 
bool HeatOn = false; 
int LeftTempature = 0;
int RightTempature = 0;

// Setup
// Set the pinModes
// Prefill arrays
// Setup the Temp Sensors
void setup() {
  //Set the mode for the heater pins
  for (int i=0; i < NumHeaterPins;i++) {
    pinMode(HeaterPin[i], OUTPUT);
  }
  pinMode(LED_BUILTIN, OUTPUT);

  for (int i=0; i < TriggerFrequency;i++) {
    SpikeHistory[i] = 0;
  }

  for (int i=0; i < NumHeaterPins;i++) {
    HeaterEnable[i] = false;
  }

  LeftSensors.begin();
  RightSensors.begin();
  LeftSensors.getAddress(LeftTemp,0);
  RightSensors.getAddress(RightTemp,0);
  LeftSensors.setResolution(LeftTemp,9);
  RightSensors.setResolution(RightTemp,9);
  
  Serial.begin(9600);
  Serial.println("--- Setup Complete ---");
}

//Main Loop
//Check the Vibration Sensor
//Read the Tempature, perform sanity checks on results
//Set the Target Tempature
//Turn heaters on or off to reach the Target Tempature
void loop() {
  bool Triggered = InputCheck();
  digitalWrite(LED_BUILTIN, LOW);
  

  if (millis() > (LastTempatureReadTime + TempatureReadDelay)){
    LastTempatureReadTime = millis();
      
    LeftSensors.requestTemperatures();
    RightSensors.requestTemperatures();
  
    LeftTempature = LeftSensors.getTempC(LeftTemp);
    RightTempature = RightSensors.getTempC(RightTemp);
  
    // If the Tempature Inputs fail sanity, end the loop
    if(TempSanityCheck(LeftTempature, RightTempature) == false){
      Serial.print("TempSanityCheck Failed - LeftTempature: ");
      Serial.print(LeftTempature);
      Serial.print(" RightTempature: ");
      Serial.println(RightTempature);
      for (int i=0; i < NumHeaterPins;i++) {
        HeaterEnable[i] = false;
      } 
      return;  
    }
  }
  
  if (Triggered) {
    HeatEnableTime = millis();
  }

  if (millis() < (HeatEnableTime+HeatOnDuration)) {
    TargetTempature = HeatTarget;
  } else {
    TargetTempature = StandByTarget;
  }

  // Left Tempature Sensor controls heaters 1&4
  // Right Tempature Sensor controls heaters 3&5
  // Heater 1 comes on when BOTH sensors are below temp
  if (LeftTempature < TargetTempature){
    HeaterEnable[0] = true;
  } else {
    HeaterEnable[0] = false;
  }
  if (RightTempature < TargetTempature){
    HeaterEnable[1] = true;
  } else {
    HeaterEnable[1] = false;
  }
  if (HeaterEnable[0] && HeaterEnable[1]){
    HeaterEnable[2] = true;
  } else {
    HeaterEnable[2] = false;
  }

  
  for (int i = 0; i < NumHeaterPins; i++){
      if (HeaterEnable[i]) {
        digitalWrite(HeaterPin[i],HIGH);
        digitalWrite(LED_BUILTIN, HIGH);
      } else {
        digitalWrite(HeaterPin[i],LOW);
      }
 
  }
  
  if (millis() > (LastDebugMsg + DebugDelay)) {
    Serial.print("TargetTemp: ");
    Serial.print(TargetTempature);
    Serial.print(" Heat Time Left: ");
    Serial.print(HeatEnableTime+HeatOnDuration - millis());
    Serial.print(" Left Status: ");
    Serial.print(LeftTempature);
    Serial.print(" ");
    Serial.print(HeaterEnable[0]);
    Serial.print(" Right Status: ");
    Serial.print(RightTempature);
    Serial.print(" ");
    Serial.print(HeaterEnable[1]);
    Serial.print(" ");
    Serial.print("Center Heater: ");
    Serial.println(HeaterEnable[2]);
    
    
    LastDebugMsg = millis();
  }
}

// Read the input value
// Make note if we have started a spike
// When the spike is over, note the time
// Compare spikes to see if we've had enough spikes withing the alloted time
// Return True if yes
bool InputCheck(){
  // This Function checks the input and determines if we should consider it triggered or not
  // This will be in two parts. The first is to check for spikes above a threshold
  // The second will be to look for multiple spikes within a time frame.
  int InputValue = analogRead(TriggerPin);
  
//  Serial.println(InputValue);
  
  if ((!SpikeUp) && (InputValue > TriggerThreshold)){
    //Spike Starting!
    SpikeUp = true;
  }

  if ((SpikeUp) && (InputValue < TriggerThreshold)) {
    // Spike completed, do the thing!
    PushDown(SpikeHistory, millis());
    SpikeUp = false;
    if ((SpikeHistory[TriggerFrequency-1] - SpikeHistory[0]) < TriggerTimeframe) {
      return true;
    }
  }
  return false;
}

//Check to see if the sensors are reporting a value that is too low to be considered valid
//Check to see that the sensors are close enough in tempature
//Testing has shown that if only one set of heaters works, the temp gradiate is big enough to fail this check
bool TempSanityCheck(int TempValue1, int TempValue2){
  // This function checks the values from the Temperature sensors to see if anything is amiss
  // A wild deviation between the two, or a value that is too low should return false
  if ((TempValue1 < LowerBound) || (TempValue2 < LowerBound)){
    return false;
  }
  if (abs(TempValue1 - TempValue2) > DeviationBound) {
    return false;
  }
  return true;
}

//Push the stack down
void PushDown(unsigned long InputArray[], unsigned long InputValue){
  int ArraySize = (sizeof(InputArray)/sizeof(unsigned long));
  for (int i = 0; i < ArraySize-1; i++) {
   InputArray[i] = InputArray[i+1];
  }
  InputArray[ArraySize-1] = InputValue;
}
