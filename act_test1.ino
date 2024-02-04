#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SS_PIN 10   // SDA pin
#define RST_PIN 9   // RST pin

#define BUTTON_PIN_ON 4   // Pin for the button to turn the relay on
#define BUTTON_PIN_OFF 5  // Pin for the button to turn the relay off

#define RELAY_PIN 6 // Pin for the relay

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create an instance of the MFRC522 library

LiquidCrystal_I2C lcd(0x27, 16, 2); // Initialize the LCD using the I2C address 0x27

byte correctCardUID[] = {0xAB, 0xCD, 0xEF, 0x12}; // Replace this with the UID of the correct card

bool isCardCorrect = false;
bool relayOn = false;

void setup() {
  Serial.begin(9600); // Initialize serial communication
  SPI.begin();        // Initialize SPI bus
  mfrc522.PCD_Init(); // Initialize MFRC522 module

  lcd.begin(16, 2);   // Initialize the LCD
  lcd.backlight();    // Turn on the backlight

  pinMode(BUTTON_PIN_ON, INPUT_PULLUP);  // Set the button pin as input with internal pull-up resistor
  pinMode(BUTTON_PIN_OFF, INPUT_PULLUP);

  pinMode(RELAY_PIN, OUTPUT); // Set the relay pin as output
  digitalWrite(RELAY_PIN, LOW); // Turn off the relay at the beginning
}

void loop() {
  // Look for new RFID cards
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // Check if the card's UID matches the correct card UID
    if (checkCardUID(mfrc522.uid.uidByte, mfrc522.uid.size)) {
      isCardCorrect = true;
      displayWelcomeMessage();
    } else {
      isCardCorrect = false;
      displayErrorMessage();
    }

    // Halt PICC (RFID card) to stop further reading until another card is present
    mfrc522.PICC_HaltA();

    // Stop reading this card
    mfrc522.PCD_StopCrypto1();
  }

  // Check if the correct card is present to control the relay
  if (isCardCorrect) {
    if (digitalRead(BUTTON_PIN_ON) == LOW) {
      turnRelayOn();
    }

    if (digitalRead(BUTTON_PIN_OFF) == LOW) {
      turnRelayOff();
    }
  }
}
}

  for (byte i = 0; i < bufferSize; i++) {
    if (buffer[i] != correctCardUID[i]) {
      return false;
    }
  }

  return true;
}

// Helper function to display the welcome message on the LCD
void displayWelcomeMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Welcome User!  ");
}

// Helper function to display the error message on the LCD
void displayErrorMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   Error: Access  ");
}

// Helper function to turn the relay on
void turnRelayOn() {
  digitalWrite(RELAY_PIN, HIGH);
  lcd.setCursor(0, 1);
  lcd.print("   Relay ON      ");
}

// Helper function to turn the relay off
void turnRelayOff() {
  digitalWrite(RELAY_PIN, LOW);
  lcd.setCursor(0, 1);
  lcd.print("   Relay OFF     ");
}
