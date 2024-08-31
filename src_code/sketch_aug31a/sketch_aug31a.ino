#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HX711.h>
#include <Keypad.h>

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = A1;  // Data output pin for HX711
const int LOADCELL_SCK_PIN = A0;   // Clock pin for HX711

const int LOADCELL1_DOUT_PIN = A5; 
const int LOADCELL1_SCK_PIN = A6;

// Create instance of HX711
HX711 scale(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); 
HX711 scale1(LOADCELL1_DOUT_PIN, LOADCELL1_SCK_PIN);  // Initialize HX711 with specified pins

// Create instance of LiquidCrystal_I2C
LiquidCrystal_I2C lcd(0x27, 16, 2); // Initialize LCD with I2C address 0x27, 16 columns and 2 rows

// Keypad configuration
const byte ROWS = 4; // Four rows in the keypad
const byte COLS = 4; // Four columns in the keypad
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
}; // Keymap for the keypad
byte rowPins[ROWS] = {2, 3, 4, 5};    // Pin connections for the keypad rows
byte colPins[COLS] = {6, 7, 8, 9};   // Pin connections for the keypad columns

// Create instance of Keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS); // Initialize keypad with specified rows, columns, and keymap

// Buzzer pin
const int buzzerPin = 13; // Pin connected to the buzzer

// Calibration factor
float calibration_factor = 196404; // Updated calibration factor for the HX711

const int numReadings = 20;
float readings1[numReadings];

int readIndex1 = 0;
float total1 = 0;
float average1 = 0;
bool tension_removed1 = false;
const float no_weight_threshold = 0.1;

// Variables to store wire type and weight
int wireType = 0; // Variable to store the selected wire type
float weight = 0.0; // Variable to store the measured weight
float length = 0.0; // Variable to store the calculated length
float lengthNeeded = 0.0; // Variable to store the length of wire needed
float remainingLength = 0.0; // Variable to store the remaining wire length

const float price0 = 50.00;
const float price1 = 70.00;
float totallength = 0;
float price = 0;

// Pin definitions for NEMA 23 motor driver
const int stepPin23 = 10; // Pin for step signal
const int dirPin23 = 11;  // Pin for direction signal
const int enablePin23 = 12; // Pin to enable/disable motor driver

// Pin definitions for NEMA 17 motor driver
const int stepPin17 = 14; // Pin for step signal
const int dirPin17 = 15;  // Pin for direction signal
const int enablePin17 = 16; // Pin to enable/disable motor driver

// Pin definition for IR sensor
const int irSensorPin = 17; // Adjust to your setup; Pin for IR sensor input

int stepsPerRevolution23 = 1600; // 200 steps per revolution * 8 microsteps per step for NEMA 23
int stepsPerRevolution17 = 200; // Assuming 3200 steps per revolution for NEMA 17 (16 microsteps)
int motorSpeed23 = 500; // Speed of the NEMA 23 motor (higher value = slower speed)
int motorSpeed17 = 200; // Speed of the NEMA 17 motor
bool motorDirection = HIGH; // Current motor direction (HIGH or LOW)

int irState = 0; // Variable to store the current state of the IR sensor
int previousIrState = 0; // Variable to store the previous state of the IR sensor
int detectionCount = 0; // Variable to count detections by the IR sensor
int targetCount = 0; // Variable to store the target count for wire dispensing

void setup() {
  Serial.begin(9600); // Initialize serial communication at 9600 baud rate
  scale.set_scale(calibration_factor);// Apply the calibration factor to the HX711
  scale.tare();
  scale1.set_scale(calibration_factor);
  scale1.tare(); // Reset the scale to 0

  for (int i = 0; i < numReadings; i++) {
    readings1[i] = 0;
  }

  Serial.println("Place known weights on the scales...");

  lcd.begin(16, 2); // Initialize the LCD with 16 columns and 2 rows
  lcd.backlight(); // Turn on the LCD backlight
  
  pinMode(buzzerPin, OUTPUT); // Set buzzer pin as output
  
  // Play different sound for welcome message
  tone(buzzerPin, 1000, 300); // Play 1kHz tone for 0.3 seconds
  delay(300); // Pause for 0.3 seconds
  tone(buzzerPin, 1500, 300); // Play 1.5kHz tone for 0.3 seconds
  delay(300); // Pause for 0.3 seconds
  
  // Display welcome message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WELCOME TO GP-15");
  delay(2000); // Display the welcome message for 2 seconds
  
  // Initialize motor pins for NEMA 23
  pinMode(stepPin23, OUTPUT);
  pinMode(dirPin23, OUTPUT);
  pinMode(enablePin23, OUTPUT);
  digitalWrite(enablePin23, LOW); // Enable the motor driver

  // Initialize motor pins for NEMA 17
  pinMode(stepPin17, OUTPUT);
  pinMode(dirPin17, OUTPUT);
  pinMode(enablePin17, OUTPUT);
  digitalWrite(enablePin17, LOW); // Enable the motor driver

  // Initialize the IR sensor pin
  pinMode(irSensorPin, INPUT); // Set IR sensor pin as input

  mainMenu(); // Display the main menu
}

void loop() {
  do{
    char key = keypad.getKey(); // Get the key pressed on the keypad
    if (key) {
      switch (key) {
        case '1':
        case '2':
          chooseWireType(key); // If '1' or '2' is pressed, choose wire type
          break;
        case 'C':
          measureWeightAndLength(); // If 'C' is pressed, measure weight and length
          break;
        default:
          break;
      }
    }
  }while(handletension()<6);
  
  // handletension();
  // if(handletension()>6){
  //   digitalWrite(stepPin23, LOW); // Step the motor
  //   delayMicroseconds(motorSpeed23); // Delay for motor speed
  // }else{
  //   lcd.clear();
  //   lcd.setCursor(0, 0);
  //   lcd.print("High tension.");
  //   lcd.setCursor(0, 1);
  //   lcd.print("Please check");
  // }
  
}

void mainMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Choose wire type");
  lcd.setCursor(0, 1);
  lcd.print("1 or 2");

  Serial.println("Choose wire type 1 or 2");
}

void chooseWireType(char key) {
  wireType = key - '0'; // Convert char to int to get the wire type

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Put wire roll");
  lcd.setCursor(0, 1);
  lcd.print("on the scale...");

  Serial.println("Put the wire roll on the scale...");

  delay(2000); // Wait for the scale to stabilize

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("C : measure");

  Serial.println("C : measure");
}

void measureWeightAndLength() {
  weight = scale.get_units(10) * (-1000); // Get average of 10 readings from the HX711 and convert to grams
  if (wireType == 1) {
    length = (weight) / 10.0; // Type 1 is 10g per meter
  } else if (wireType == 2) {
    length = (weight) / 8.0; // Type 2 is 8g per meter
  }

  remainingLength = length; // Initialize remaining length

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Weight: ");
  lcd.print(weight, 2); // Print weight with two decimal places
  lcd.print(" g");

  lcd.setCursor(0, 1);
  lcd.print("Length: ");
  lcd.print(length, 2); // Print length with two decimal places
  lcd.print(" m");

  Serial.print("Weight of wire roll: ");
  Serial.print(weight, 2); // Print weight with two decimal places
  Serial.println(" g");
  Serial.print("Length of wire roll: ");
  Serial.print(length, 2); // Print length with two decimal places
  Serial.println(" m");

  delay(2000); // Delay for 2 seconds before next step
  displayNecesity();
}

void enterLengthNeeded() {
  String lengthNeededStr = ""; // Initialize string to store the length needed

  while (true) {
    char key = keypad.getKey(); // Get the key pressed on the keypad

    if (key) {
      if (key == '#') { // If '#' is pressed, confirm the input
        lengthNeeded = lengthNeededStr.toFloat(); // Convert string to float

        if (lengthNeeded > remainingLength) { // Check if the requested length exceeds the remaining length
          // Buzz the buzzer three times
          for (int i = 0; i < 5; i++) {
            tone(buzzerPin, 1000, 200); // 1kHz tone for 0.2 seconds
            delay(100); // Pause for 0.2 seconds
          }

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Length too long!");
          Serial.println("Length too long! A: Change roll");

          delay(3000);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("A: Change roll");
          lcd.setCursor(0, 1);
          lcd.print("B: Enter pos.len");
          Serial.println("A: Change roll B: Enter pos.len");

          while (true) {
            key = keypad.getKey();
            if (key == 'A') {
              mainMenu();
              return;
            } else if (key == 'B') {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("enterlength needed:");
              enterLengthNeeded();
              return;
            }
          }
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Length accepted");
          lcd.setCursor(0, 1);
          lcd.print("Proceeding...");
          Serial.println("Length accepted, proceeding...");
          delay(1000);
          // Proceed with motor control
          targetCount = ((lengthNeeded * 100) - 37.5) / 12.5; // Calculate target count based on the length needed
          dispenseWire(targetCount); // Dispense the wire
          return;
        }
      } else if (key == 'D') { // If 'D' is pressed, delete the last character
        if (lengthNeededStr.length() > 0) {
          lengthNeededStr.remove(lengthNeededStr.length() - 1); // Remove last character
          lcd.setCursor(0, 1);
          lcd.print("                "); // Clear the line
          lcd.setCursor(0, 1);
          lcd.print(lengthNeededStr);
        }
      } else {
        if (key == '*') {
          key = '.'; // Replace '*' with '.'
        }
        lengthNeededStr += key;
        lcd.setCursor(0, 1);
        lcd.print(lengthNeededStr);
      }
    }
  }
}

void dispenseWire(int targetCount) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Target: ");
  lcd.print(lengthNeeded);
  lcd.print(" m");
  lcd.setCursor(0, 1);
  lcd.print("Winding...");
  delay(5000);

  detectionCount = 0; // Reset detection count

  while (detectionCount < targetCount) {
    irState = digitalRead(irSensorPin); // Read the state of the IR sensor
    if (irState == LOW && previousIrState == HIGH) {
      detectionCount++;
      Serial.print("Object detected! Count: ");
      Serial.println(detectionCount);
      float temp = (detectionCount*12.5)+50;
      lcd.setCursor(0, 1);
      lcd.print("Length: ");
      lcd.print(temp);
      lcd.print(" cm");
    }
    previousIrState = irState; // Update the previous state
    if(handletension()<6){
      digitalWrite(stepPin23, HIGH); // Step the motor
      delayMicroseconds(motorSpeed23); // Delay for motor speed
      digitalWrite(stepPin23, LOW); // Step the motor
      delayMicroseconds(motorSpeed23); // Delay for motor speed
    }else{
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("High tension.");
      lcd.setCursor(0, 1);
      lcd.print("Please check");
    }
    

    if (remainingLength <= 0) { // If remaining length is less than or equal to 1
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wire roll is empty");
      Serial.println("Wire roll is empty");
      tone(buzzerPin, 1000, 1000); // Beep for 1 second
      delay(1000);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Place a new roll");
      lcd.setCursor(0, 1);
      lcd.print("Press A to continue");

      bool waitingForInput = true;
      while (waitingForInput) {
        char key = keypad.getKey();
        if (key == 'A') {
          waitingForInput = false;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("New roll placed");
          delay(1000); // Briefly show the message
          remainingLength = 0; // Reset remaining length for the new roll
          lengthNeeded = 0; // Reset length needed
          mainMenu(); // Return to the main menu or proceed to measurement
        }
      }
    }
  }

  remainingLength -= lengthNeeded; // Update the remaining length

  if (remainingLength <= 1) { // If no wire is remaining
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Wire roll empty");
    Serial.println("Wire roll empty");
    tone(buzzerPin, 1000, 1000); // Beep for 1 second
    delay(1000);
    mainMenu(); // Return to main menu
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("More wire?");
    lcd.setCursor(0, 1);
    lcd.print("A:Yes B:Cut Wire");

    bool waitingForInput = true;
    while (waitingForInput) {
      char key = keypad.getKey();
      if (key == 'A') {
        waitingForInput = false;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter length (m):");
        lengthNeeded = 0.0; // Reset length needed
        enterLengthNeeded();
      } else if (key == 'B') {
        waitingForInput = false;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Cutting the Wire.");
        delay(1000);
        rotateMotor17(8); // Rotate NEMA 17 motor 8 rounds
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Price : Rs.");
        totallength = length - remainingLength;
        if(wireType == '1'){
          price = price1 * totallength;
        }else{
          price = price0 * totallength;
        }
        lcd.print(price);
        lcd.setCursor(0, 1);
        lcd.print("Length: ");
        lcd.print(totallength);
        delay(2000);
        lengthNeeded = 0.0; // Reset length needed
        enterLengthNeeded();
        mainMenu();
      }
    }
  }
}

void rotateMotor17(int rounds) {
  digitalWrite(enablePin17, LOW); // Enable the motor driver
  digitalWrite(dirPin17, motorDirection); // Set motor direction

  int totalSteps = rounds * stepsPerRevolution17; // Calculate total steps based on rounds

  for (int i = 0; i < totalSteps; i++) {
    digitalWrite(stepPin17, HIGH); // Step the motor
    delayMicroseconds(motorSpeed17); // Delay for motor speed
    digitalWrite(stepPin17, LOW); // Step the motor
    delayMicroseconds(motorSpeed17); // Delay for motor speed
  }

  digitalWrite(enablePin17, HIGH); // Disable the motor driver
}

void displayNecesity(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Hang wire Roll");
  lcd.setCursor(0,1);
  lcd.print("A: Continue!");
  bool waitingForInput = true;
  while (waitingForInput) {
    char key = keypad.getKey();
    if (key == 'A') {
      waitingForInput = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter len.needed");
      Serial.println("Enter len.needed");
      enterLengthNeeded(); // Prompt the user to enter the length needed;
    } else {
      continue;
    }
  }
}

float handletension(){
  total1 = total1 - readings1[readIndex1];
  readings1[readIndex1] = scale1.get_units(5); // Get average of 5 readings from the HX711 for Load Cell 1
  total1 = total1 + readings1[readIndex1];
  readIndex1 = readIndex1 + 1;
  if (readIndex1 >= numReadings) {
    readIndex1 = 0;
  }
  average1 = total1 / numReadings;
  float weight1 = average1 * 10; 
  if(weight1<0){
    weight1*=-1;
  }

  // Serial.print(" | Measured tension: ");
  // Serial.print(weight1, 4);
  // Serial.println(" N");
  return(weight1);
}