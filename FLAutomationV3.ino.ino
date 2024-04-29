// Arduino Mega 2560

#include <LiquidCrystal_I2C.h>//Header file for LCD from https://www.arduino.cc/en/Reference/LiquidCrystal

#include <Keypad.h>  //Header file for Keypad from https://github.com/Chris--A/Keypad
#include <Stepper.h>
#include <Adafruit_VL53L1X.h>
#include <CFRotaryEncoder.h> 

// DEFINES
// Define stepper motor connections:
#define dirPin 22
#define stepPin 24
#define zStopUpper 26
#define zStopLower 28
// VL53X
#define IRQ_PIN 2
#define XSHUT_PIN 3
const int stepsPerRevolution = 200;  // change this to fit the number of steps per revolution
const int rpm  = 60
const byte ROWS = 5;  // Four rows
const byte COLS = 4;  // Three columns

// LS pitch = 1.25 mm/rev
// 1/16 uStep 1.8 Deg 1:1 2560.00
const long STEPS_PER_MM_8 = 1280.00;   // 1/8 step 1.8 deg @ 1:1 ratio
const long STEPS_PER_MM_16 = 2560.00;  // 1/16 uStep 1.8 Deg 2560.00
// Lense configs
// Setting the focal length for each lense
const long L200 = 385;
const long L175 = 302;
const long L70 = 140;

// Rotary encoder pins
const int ROT_PIN_OUTPUT_A = 39;
const int ROT_PIN_OUTPUT_B = 41;
const int ROT_PIN_PUSH_BUT = 43;

// Menu system



// initialize the rotary encoder
CFRotaryEncoder rotaryEncoder(ROT_PIN_OUTPUT_A, ROT_PIN_OUTPUT_B, ROT_PIN_PUSH_BUT);   

// initialize the stepper library on pins 8 through 11:
Stepper zStepper(stepsPerRevolution, 8,9,10,11);

// Init the VL53L1X
Adafruit_VL53L1X vl53 = lidar(XSHUT_PIN, IRQ_PIN);

// Define the Keymap
char keys[ROWS][COLS] = {
  { '*', '#', 'F', 'S' },  // F = F2 focus  S = F1 Stepper configs
  // { 'F1', 'F2', '#', '*' },
  // { '1', '2', '3', 'Up' },
  { 'U', '3', '2', '1' },
  //{ '4', '5', '6', 'Dn' },
  { 'D', '6', '5', '4' },
  //{ '7', '8', '9', 'Esc' },
  { '-', '9', '8', '7' },
  //{ 'Left', '0', 'Right', 'Enter' }
  { '+', 'R', '0', 'L' }  //Ent = + Esc = -
};

byte rowPins[ROWS] = { 22, 24, 26, 28, 30 };                          //{ 2, 3, 4, 5, 6 };                               // Connect keypad ROW0, ROW1, ROW2 and ROW3 to these Arduino pins.
byte colPins[COLS] = { 32, 34, 36, 38 };                              //{ 8, 9, 10, 11 };                                // Connect keypad COL0, COL1 and COL2 to these Arduino pins.
Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);  //  Create the Keypad

LiquidCrystal_I2C lcd(0x27, 20, 4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

char key, action;
enum { SetLense,
       SetStepper,
       RunJob } ProgramMode;
int programMode = RunJob;
long Number;
long zCurrent;
long currentLensFL = L175;         //default to the 175 lense for now
long stepsPerMM = STEPS_PER_MM_8;  //default to 1/8 step for now



boolean result = false;
boolean blink = false;
boolean ledPin_state;

void setup() {
  // set the speed at 60 rpm:
  zStepper.setSpeed(60);  
  Serial.begin(115200);
  
  while (!Serial) delay(10);
  
  lcd.init();  // initialize the lcd
  lcd.backlight();
  lc.leftToRight();
  lcd.noAutoscroll();
  lcd.print("Initializing...");
  ledPin_state = digitalRead(LED_BUILTIN);
  kpd.addEventListener(keypadEvent);

  delay(5000);  //Wait for display to show info
  clearScreen()  //Then clean it
   // Define callbacks.
   rotaryEncoder.setAfterRotaryChangeValueCallback(rotaryAfterChangeValueCallback);
   rotaryEncoder.setPushButtonOnPressCallback(rotaryOnPressCallback);
  
    
  Wire.begin();
  say("Initializing Lidar");
  if (! lidar.begin(0x29, &Wire)) {
    say("Error on init of VL sensor: ");
    say(lidar.vl_status);
    while (1)       delay(10);
  }
  say("Lidar sensor OK!");
  say("Sensor ID: 0x");
  say(lidar.sensorID());

  //check to ensure lidar is working
  if (! lidar.startRanging()) {
    say("Lidar Ranging failed:");
    say(lidar.vl_status);
    while (1)
      delay(10);
  }
  say("Ranging started");
  // turn off ranging and only run it when the focus function is called
  lidar.stopRanging(); 
  // Valid timing budgets: 15, 20, 33, 50, 100, 200 and 500ms!
  lidar.setTimingBudget(50);
  Serial.print(F("Timing budget (ms): "));
  Serial.println(lidar.getTimingBudget());
  delay(30);
  clearScreen();   
  mainMenu();
  
  
}

void loop() {
 rotaryEncoder.loop();
}

void say(String message){
  lcd.println(message);
}

void clearScreen(){
  lcd.clear();
}

void keypadEvent(KeypadEvent key) {
  switch (kpd.getState()) {
    case PRESSED:
      if (key == 'S') {
        programMode = SetLense;
        say("Program in is Lense Configuration mode");
      }

      if (key == '#') {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        break;

        case RELEASED:
          if (key == '*') {
            digitalWrite(LED_BUILTIN, ledPin_state);  // Restore LED state from before it started blinking.
            blink = false;
          }
          break;

        case HOLD:
          if (key == '*') {
            blink = true;  // Blink the LED when holding the * key.
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
          }
          break;
      }
  }
}

void setFocalLen(long focalLen) {
  currentLensFL = focalLen;
}

void setFocus(){
  // if (lidar > currentFL) lower the z
  // else raise the z to currentFL  
  if (lidar.startRanging() && lidar.dataReady()){
    while(true){
      if(lidar.distance > currenLensFL){
        stepCW(1);
      }
      else{
        stepCCW(1);
      }
    }
  }
  else{
    //throw an  error message to the display
    say(
  }
}

void setStepMode() {
  if (Number == 8 | Number == 16) {
    stepsPerMM = Number == 8 ? STEPS_PER_MM_8 : STEPS_PER_MM_16;
  }
}

void zeroZ() {
  lcd.println("This function will set the z height zero position");
  // lower z to lower limit
  #read the  pin in a loop and exec 
  digitalWrite(dirPin, HIGH);
  while (digitalRead(zStopLower) == LOW){
    zStepper.step(stepsPerRevolution);  
  }  
}

void setZ(int zHeight) {
  lcd.println("This function will set the z height");
}

void stepCW(int steps) {
  for (int i = 0; i < steps; i++) {
    zStepper.step(1);
  }
}

void stepCCW(int steps) {
  for (int i = 0; i < steps; i++) {
  zStepper.step(-1);  }
}

void rotaryAfterChangeValueCallback() {
  // 1. check which screen we are on
  // 2. if a menu screen 
  
  //if(rotaryEncoder.getValue());
}

void rotaryOnPressCallback() {
    Serial.println("Button was pressed.");
}

// void DisplayResult() {
//   lcd.setCursor(0,
//                 0);  // set the cursor to column 0, line 1
//   lcd.print(Num1);
//   lcd.print(action);
//   lcd.print(Num2);

//   if (result == true) {
//     lcd.print(" =");
//     lcd.print(Number);
//   }
//   //Display the result

//   lcd.setCursor(0, 1);  // set the cursor to column  0, line 1
//   lcd.print(Number);    //Display the result
// }
