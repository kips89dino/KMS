#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ezAnalogKeypad.h>

#define SS_PIN 9
#define RST_PIN 10

MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
ezAnalogKeypad keypad(A0);

// User structure
struct User {
  byte uid[4]; // UID of the user
  String username; // Username of the user
  String password; // Password of the user
};

// UID and corresponding user data
const byte NUM_USERS = 4;
User users[NUM_USERS] = {
  {{0x27, 0x6A, 0x5E, 0xB5}, "Aisyah", "1234"},
  {{0x75, 0x33, 0x07, 0xAD}, "Aqmar", "5678"},
  {{0x5C, 0x78, 0xB1, 0x33}, "Akif", "4321"},
  {{0x71, 0x9F, 0x74, 0x27}, "Rahman", "8765"}
};

byte userUID[4]; // Variable to store the read UID
String enteredPassword; // Variable to store the entered password

enum AuthState {
  CARD_TAG,
  PASSWORD
};

AuthState authState = CARD_TAG; // Initial authentication state is card tag

void setup() {
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522

  lcd.init(); // Initialize the LCD
  lcd.backlight();

  initializeKeypad();

  Serial.println("Tap RFID/NFC Tag on reader");
  prompt_rfid();
}

void loop() {
  checkAccess();
  processKeypad();
}

void prompt_rfid() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tap RFID/NFC Tag");
  lcd.setCursor(0, 1);
  lcd.print("on reader");
}

// Read UID from RFID card
void readUID() {
  memcpy(userUID, rfid.uid.uidByte, 4);
}

// Compare two UID arrays
bool compareUID(byte* uid1, byte* uid2) {
  for (int i = 0; i < 4; i++) {
    if (uid1[i] != uid2[i]) {
      return false;
    }
  }
  return true;
}

void initializeKeypad() {
  keypad.setNoPressValue(1023);  // analog value when no key is pressed
  keypad.registerKey('1', 954); // analog value when the key '1' is pressed
  keypad.registerKey('2', 923); // analog value when the key '2' is pressed
  keypad.registerKey('3', 849); // analog value when the key '3' is pressed
  keypad.registerKey('A', 68); // analog value when the key 'A' is pressed
  keypad.registerKey('4', 958); // analog value when the key '4' is pressed
  keypad.registerKey('5', 931); // analog value when the key '5' is pressed
  keypad.registerKey('6', 873); // analog value when the key '6' is pressed
  keypad.registerKey('B', 519); // analog value when the key 'B' is pressed
  keypad.registerKey('7', 962); // analog value when the key '7' is pressed
  keypad.registerKey('8', 939); // analog value when the key '8' is pressed
  keypad.registerKey('9', 892); // analog value when the key '9' is pressed
  keypad.registerKey('C', 683); // analog value when the key 'C' is pressed
  keypad.registerKey('*', 965); // analog value when the key '*' is pressed
  keypad.registerKey('0' , 945); // analog value when the key '0' is pressed
  keypad.registerKey('#', 906); // analog value when the key '#' is pressed
  keypad.registerKey('D', 764); // analog value when the key 'D' is pressed
  // ... register more keys if needed
}

void checkAccess() {
  if (rfid.PICC_IsNewCardPresent() && authState == CARD_TAG) { // New tag is available and in card tag state
    if (rfid.PICC_ReadCardSerial()) { // NUID has been read
      readUID();
      Serial.print("Scanned UID: ");
      for (int i = 0; i < 4; i++) {
        Serial.print(userUID[i] < 0x10 ? "0" : "");
        Serial.print(userUID[i], HEX);
      }
      Serial.println();
      rfid.PICC_HaltA(); // Halt PICC
      rfid.PCD_StopCrypto1(); // Stop encryption on PCD
      promptPassword();
    }
  }
}

void promptPassword() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter password:");
  lcd.setCursor(0, 1);
  lcd.print(">");
  enteredPassword = "";
  authState = PASSWORD; // Switch to password authentication state
}

void processKeypad() {
  if (authState == PASSWORD) {
    char key = keypad.getKey();

    if (key != '\0' && key != '#' && key != '*' && key != 'D') {
      enteredPassword += key;
      lcd.setCursor(1,1);
      lcd.print(enteredPassword);
    } else if (key == '#' && authState == PASSWORD) {
      validatePassword();
    } else if (key == '*' && authState == PASSWORD) {
      clearPassword();
    } else if (key == 'D' && authState == PASSWORD && enteredPassword != "NULL") {
      deletePreviousCharacter();
    }
  }
}

void clearPassword() {
  enteredPassword = "";
  lcd.setCursor(0, 1);
  lcd.print("                "); // Clear the password display
  lcd.setCursor(0, 1);
  lcd.print(">");
}

void deletePreviousCharacter() {
  if ((enteredPassword.length() > 0) && (enteredPassword != "NULL")) {
    lcd.setCursor(enteredPassword.length(), 1);
    lcd.print(" ");
    enteredPassword.remove(enteredPassword.length() - 1);
    lcd.setCursor(enteredPassword.length(), 1);
  }else{
    clearPassword();
    lcd.setCursor(1,1);
  }
}

void validatePassword() {
  bool accessGranted = false;
  String username;

  for (int i = 0; i < NUM_USERS; i++) {
    if (compareUID(userUID, users[i].uid)) {
      if (enteredPassword == users[i].password) {
        accessGranted = true;
        username = users[i].username;
      }
      break;
    }
  }

  if (accessGranted) {
    lcd.setCursor(0, 0);
    lcd.print("Access Granted");
    lcd.setCursor(0, 1);
    lcd.print("Welcome, " + username);

    // check keys availability and record
    // servo open
    // user take key
    //servo close
    // check keys
    // send info to internet

    


  } else {
    lcd.setCursor(0, 0);
    lcd.print("Access Denied");
    lcd.setCursor(0, 1);
    lcd.print("Invalid Password");
  }

  delay(2000);
  lcd.clear();
  authState = CARD_TAG; // Switch back to card tag state
  prompt_rfid();
}