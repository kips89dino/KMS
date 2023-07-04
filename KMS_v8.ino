/*/////////////////////////////////////////////////////////////////////////////////////
//  pin for keypad: orange- A0, red-ground, yellow- Vcc/5V/3.3V
//  
//  pin for MFCR: SCK - d13, MOSI - 11, MOSI -12, RST - 10, SDA - 9
//
//  pin for lcdi2c: sda slc 5v gnd on the breakboard ...please do not use pin A4 & A5
//
//  pin esp-01 module(on nano) : rx - 2, tx - 3 if use NodemCu still the same pin 
//
/////////////////////////////////////////////////////////////////////////////////////*/

#include <ezAnalogKeypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <SoftwareSerial.h>


#define RST_PIN 10 // reset pin for rfid
#define SS_PIN 9  // SDA pin for rfid

SoftwareSerial NodeMCU(2, 3); // RX, TX pins#define SS_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
ezAnalogKeypad keypad(A0);

int currentOption = 1;
int totalOptions = 4;
bool optionSelected = false;
const int kunciCount = 3; //bil kunci
char kunciStatus[kunciCount] = {'0', '0', '0'/*, '0', '0'*/};
bool sensorState[kunciCount] = {false, false, false/*, false, false*/};
bool prevSensorState[kunciCount] = {false, false, false/*, false, false*/};
String Kunci;
String waitForResponse(String expectedResponse, unsigned long timeout = 5000);
String sendCommand(String command, String expectedResponse);

struct User {
  byte uid[4];
  String username;
  String password;
  String Kunci;
};

const byte NUM_USERS = 4;
User users[NUM_USERS] = {
  {{0x27, 0x6A, 0x5E, 0xB5}, "Aisyah", "1234", ""},
  {{0x75, 0x33, 0x07, 0xAD}, "Aqmar", "5678", ""},
  {{0x5C, 0x78, 0xB1, 0x33}, "Akif", "9012", ""},
  {{0x71, 0x9F, 0x74, 0x27}, "Rahman", "3456", ""}
  
};

String enteredPassword;

enum AuthState {
  VALID,
  INVALID
};
enum AuthProccess {
  CARD_TAG,
  PASSWORD
};

enum Door {
  OPEN,
  CLOSE
};

Servo myservo;
Door door = CLOSE;
AuthState authState = INVALID;
AuthProccess authProccess = CARD_TAG;
byte userUID[4];

void setup() {
  //Serial.begin(9600);
  NodeMCU.begin(9600);
  lcd.init();
  lcd.backlight();

  registerKeys();

  SPI.begin();
  rfid.PCD_Init();

  // Set the sensor pins as inputs
  pinMode(4, INPUT);
  pinMode(5, INPUT);
  pinMode(6, INPUT);
  //pinMode(5, INPUT);
  //pinMode(6, INPUT);

  myservo.attach(A2);  
  DoorClose();

  lcd.setCursor(0, 0);
  lcd.print("Key Management");
  lcd.setCursor(0, 1);
  lcd.print("System");
  delay(2000);
  displayMenu();
}

void loop() {
  unsigned char key = keypad.getKey();
  if (key) {
    if (optionSelected) {
      if (key == 'B') {
        optionSelected = false;
        displayMenu();
      }
    } else {
      if (key == 'A') {
        optionSelected = true;
        performMenuAction(currentOption);
      } else if (key == 'B') {
        currentOption = constrain(currentOption - 1, 1, totalOptions);
        displayMenu();
      } else if (key == 'C') {
        currentOption = constrain(currentOption + 1, 1, totalOptions);
        displayMenu();
      }
    }
  }
} // end void menu

void displayMenu() {
  lcd.clear();
  lcd.print("Menu Options:");
  lcd.setCursor(0, 1);
  if(currentOption == 1){
    lcd.print("Borrow/Return");
  }
  else if(currentOption == 2){
    lcd.print("Key Status");
  }
  else if(currentOption == 3){
    lcd.print("Keypad Setting");
  }
  else if(currentOption == 4){
    lcd.print("Help");  
  } 
}

void performMenuAction(int option) {
  lcd.clear();
  lcd.print("Performing Option:");
  lcd.setCursor(0, 1);
  lcd.print(option);
  delay(2000);

  switch (option) {
    case 1:
      OpenSafe();
      break;
    case 2:
      // Option 2 action
      checkKeyStat();
      break;
    case 3:
      // Option 3 action
      CheckKeypad();
      break;
    case 4:
      // Option 4 action
      Help();
      break;
  }

  optionSelected = false;
  displayMenu();
}

void OpenSafe() {
  unsigned char key = keypad.getKey();
  authState = INVALID; // reset state read card
  memset(userUID, 0, sizeof(userUID)); // reset byte memory
  String info1 =""; // data to be send to internet
  String info2 ="";
  String info3 ="";
  String info ="";

  lcd.clear();
  lcd.print("Tap RFID/NFC Tag");
  lcd.setCursor(0, 1);
  lcd.print("on reader");
  
  while (authState == INVALID ) {// reading process
    unsigned char key = keypad.getKey();
    if (rfid.PICC_IsNewCardPresent()) {
      if (rfid.PICC_ReadCardSerial()) {
        memcpy(userUID, rfid.uid.uidByte, 4);
      }
      for (int i = 0; i < NUM_USERS; i++){// compare card with register user
        if (compareUID(userUID, users[i].uid)) {    // user exist

          info1 += users[i].username;        // get user name and display
          lcdwelcome(info1);           
          users[i].Kunci = manageKeys();    // save keystat to user and display
          info2 += users[i].Kunci;
          lcdKeyBR(info2);
          authState = VALID;              // break loop
          info = info1+"&"+info2;
          DoorOpen();
          break;
        }
      }
    }
    if(key == 'B'){ // exit option
      authState = VALID;
    }  
  }
  while(door == OPEN){
    
    unsigned char key = keypad.getKey();
    
    info2 = manageKeys();
    updateKunci(info1,info2);
    lcdWarningDO();
    if(key == 'C'){
      DoorClose();
      info3 = "CLOSE";
      info = "&COM&"+info1 +"&"+ info2 +"&" +info3;
      lcdKeyBR(info2);
      Serial.println(info);
      NodeMCU.println(info);
      break;
    }
    info = info1 +"&"+ info2;
    Serial.println(info);
  delay(2000);  
  }
         
  Serial.println(info);
}

void CheckKeypad(){

  while(1){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Pin value:");
    lcd.setCursor(0,1);
    lcd.print(analogRead(A0));
    delay(100);
  }
}

void checkKeyStat() {
    String myString = manageKeys();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Key Available :");
    lcd.setCursor(1,1);
    lcd.print(myString);
    delay(5000);
}

void Help() {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("KMS Guide :");
    lcd.setCursor(1,1);
    lcd.print("Key Borrow = 0");
    delay(5000);
    lcd.setCursor(1,1);
    lcd.print("Key Return = 1");
    delay(5000);
}

bool compareUID(byte* uid1, byte* uid2) {
  for (byte i = 0; i < 4; ++i) {
    if (uid1[i] != uid2[i]) {
      return false;
    }
  }
  return true; 
}

void registerKeys(){ // keypad value register
  keypad.setNoPressValue(0);
  registerKey('1', 873);
  registerKey('2', 175);
  registerKey('3', 95);
  registerKey('A', 67);
  registerKey('4', 470);
  registerKey('5', 149);
  registerKey('6', 87);
  registerKey('B', 62);
  registerKey('7', 324);
  registerKey('8', 130);
  registerKey('9', 80);
  registerKey('C', 59);
  registerKey('*', 248);
  registerKey('0', 116);
  registerKey('#', 74);
  registerKey('D', 55);
  keypad.setDebounceTime(500);
}

void registerKey(char key, int analogValue) {
  keypad.registerKey(key, analogValue);
}

String manageKeys() {
  int b, r;
  char cts[33];
  Kunci = "";
  // Read the sensor states
  for (int i = 0; i < kunciCount; i++) {
    prevSensorState[i] = sensorState[i];
    sensorState[i] = digitalRead(i + 4);
  }

  for (int i = 0; i < kunciCount; i++) {
    // Check for change in sensor state
    if (sensorState[i] != prevSensorState[i]) {
      if (sensorState[i]) {
        // kunci is not present, mark it as returned
        if (kunciStatus[i] == '1') {
          kunciStatus[i] = '0';
          //Serial.print("kunci ");
          //Serial.print(i + 1);
          //Serial.println(" returned");
        }
      } else {
        // kunci is present, mark it as borrowed
        if (kunciStatus[i] == '0') {
          kunciStatus[i] = '1';
          //Serial.print("kunci ");
          //Serial.print(i + 1);
          //Serial.println(" borrowed");
        }
      }
    }
  }

  // Display kunci status after each iteration
  //Serial.print("kunci Status: ");
  for (int i = 0; i < kunciCount; i++) {
    //Serial.print(kunciStatus[i]);
    //lcd.setCursor(i,1);
    //lcd.print(kunciStatus[i]);
    Kunci +=  kunciStatus[i];
  }
  delay(1000);
  return Kunci;
}

void updateKunci(const String& username, const String& newKunci) {
  for (int i = 0; i < NUM_USERS; i++) {
    if (users[i].username == username) {
      users[i].Kunci = newKunci;
      break;  // Exit the loop once the username is found and updated
    }
  }
}


void servoPost(int *val){  
  myservo.write(val);           // sets the servo position according to the scaled value
  delay(200);                   // waits for the servo to get there
}

void lcdwelcome(String name){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Welcome,");
  lcd.setCursor(0, 1);
  lcd.print(name);
  delay(1000);
}

void lcdclose(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Invalid Card");
  lcd.setCursor(0, 1);
  lcd.print(" Door Close");
}

void lcdKeyBR(String Stat){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Key Status");
  lcd.setCursor(0, 1);
  lcd.print(Stat);
  delay(1000);
}

void lcdWarningDO(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Close Door");
  lcd.setCursor(0, 1);
  lcd.print("Press C");
  delay(1000);
}

void DoorOpen(){
  door = OPEN;
  servoPost(30);
  delay(200);
}

void DoorClose(){
  door = CLOSE;
  servoPost(108);
  delay(200);
}