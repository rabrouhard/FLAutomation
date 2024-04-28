// Arduino Mega 2560

#include <LiquidCrystal_I2C.h>
//Header file for LCD from https://www.arduino.cc/en/Reference/LiquidCrystal
#include <Keypad.h>  //Header file for Keypad from https://github.com/Chris--A/Keypad

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

// Define stepper motor connections:
#define dirPin 22
#define stepPin 24
#define zStopUpper 26
#define zStopLower 28

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
  Serial.begin(9600);
  lcd.init();  // initialize the lcd
  lcd.backlight();
  digitalWrite(dirPin, HIGH);
  lcd.autoscroll();
  lcd.print("Initializing...");
  ledPin_state = digitalRead(LED_BUILTIN);
  kpd.addEventListener(keypadEvent);

  delay(5000);  //Wait for display to show info

  lcd.clear();  //Then clean it
}

void loop() {

  key = kpd.getKey();
  //storing pressed key value in a char

  if (key != NO_KEY) {
    Serial.print(key);
    //DetectButtons();
  }
  // DetectButtons();

  // if (result == true){
  //   CalculateResult();
  // }

  //  DisplayResult();
}

void say(String message){
  
  lcd.println(message);

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

void setStepMode() {
  if (Number == 8 | Number == 16) {
    stepsPerMM = Number == 8 ? STEPS_PER_MM_8 : STEPS_PER_MM_16;
  }
}

void zeroZ() {
  lcd.println("This function will set the z height zero position");
  // lower z to lower limit
  setZ(0);
  setZ(currentLensFL);
}

void setZ(int zHeight) {
  lcd.println("This function will set the z height");
  int steps = stepsPerMM * zHeight;
  //  int stepsToMove = getSteps(zHeight);
  if (zCurrent > zHeight) {
    stepCCW(steps);
  } else {
    stepCW(steps);
  }
}

void stepCW(int steps) {
  // Set the spinning direction clockwise:
  digitalWrite(dirPin, HIGH);
  // Spin the stepper motor 5 revolutions fast:
  for (int i = 0; i < steps; i++) {
    // These four lines result in 1 step:
    digitalWrite(stepPin, HIGH);
  }
}

void stepCCW(int steps) {
  // Set the spinning direction counterclockwise:
  digitalWrite(dirPin, LOW);
  for (int i = 0; i < steps; i++) {
    // These four lines result in 1 step:
    digitalWrite(stepPin, HIGH);
  }
}

void DetectButtons() {
  lcd.clear();  //Then clean it

  if (key == '+')
  //If cancel Button is pressed
  {
    Serial.println("Button Enter");
    lcd.println("Enter Button Pressed");
    setZ(Number);
    result = false;
  }

  if (key == 'L')
  //If cancel Button is pressed
  {
    Serial.println("Button Left");
    lcd.println("Left Button Pressed");
    result = false;
  }

  if (key == 'R')
  //If cancel Button is pressed
  {
    Serial.println("Button Right");
    lcd.println("Right Button Pressed");
    result = false;
  }

  if (key == '-')
  //If cancel Button is pressed
  {
    Serial.println("Button Esc");
    lcd.println("Esc Button Pressed");
    result = false;
  }

  if (key == 'F')
  //F2 Sets the
  {
    Serial.println("Button F2");
    lcd.println("Setting the FL for the lense");
    if (Number == L175 | Number == L200 | Number == L70) {
      setFocalLen(Number);
    }
    result = false;
  }

  if (key == 'S')
  //If cancel Button is pressed
  {
    Serial.println("Button F1");
    lcd.println("Setting the micro step mode");
    if (Number == 8 | Number == 16) {
      setStepMode();
    }

    result = false;
  }

  if (key == 'U')
  //If cancel Button is pressed
  {
    Serial.println("Button Up");
    lcd.println("Up Button Pressed");
    result = false;
  }

  if (key == 'D')
  //If cancel Button is pressed
  {
    Serial.println("Button Dn");
    lcd.println("Down Button Pressed");
    result = false;
  }
  if (key == '*')  //If cancel Button is pressed
  {
    Serial.println("Button *");
    lcd.println("* Button Pressed");
    result = false;
  }

  if (key == '1')  //If Button 1 is pressed
  {
    Serial.println("Button 1");
    if (Number == 0)
      Number = 1;
    else
      Number = (Number * 10) + 1;  //Pressed twice
  }

  if (key == '4') {
    Serial.println("Button 4");
    if (Number == 0)

      Number = 4;
    else
      Number = (Number * 10) + 4;  //Pressed twice
  }


  if (key == '7') {
    Serial.println("Button 7");
    if (Number == 0)
      Number = 7;
    else
      Number = (Number * 10) + 7;  //Pressed twice
  }


  if (key == '0') {
    Serial.println("Button 0");  //Button 0 is Pressed
    if (Number == 0)
      Number = 0;

    else
      Number = (Number * 10) + 0;  //Pressed twice
  }

  if (key == '2')  //Button 2 is Pressed
  {
    Serial.println("Button 2");
    if (Number == 0) {
      Number = 2;
    } else {
      Number = (Number * 10) + 2;
    }
  }

  if (key == '5') {
    Serial.println("Button 5");
    if (Number == 0) {
      Number = 5;
    } else {
      Number = (Number * 10) + 5;
    }
  }

  if (key == '8') {
    Serial.println("Button 8");
    if (Number == 0) {
      Number = 8;
    } else {
      Number = (Number * 10) + 8;  //Pressed twice
    }
  }


  if (key == '#') {
    Serial.println("Button #");
    lcd.println("# Button Pressed");
    result = true;
  }

  if (key == '3') {
    Serial.println("Button 3");

    if (Number == 0) {
      Number = 3;
    } else {
      Number = (Number * 10) + 3;
    }
  }

  if (key == '6') {
    Serial.println("Button 6");
    if (Number == 0) {
      Number = 6;
    } else {
      Number = (Number * 10) + 6;  //Pressed twice
    }
  }

  if (key == '9')

  {
    Serial.println("Button 9");
    if (Number == 0) {
      Number = 9;
    } else {
      Number = (Number * 10) + 9;  //Pressed twice
    }
  }
}



// void CalculateResult() {

//   if (action == '+')
//     Number = Num1 + Num2;

//   if (action == '-')
//     Number = Num1 - Num2;

//   if (action == '*')
//     Number = Num1 * Num2;

//   if (action == '/')

//     Number = Num1 / Num2;
// }

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
