#include <AES.h>
#include <CBC.h>
#include <Crypto.h>
#include <string.h>
#include <SHA256.h>

#include <RH_ASK.h>
#include <SPI.h> // Not actualy used but needed to compile
//#pragma pack 1
RH_ASK driver(2000, 2, 8, 10); // ESP8266 or ESP32: do not use pin 11
#define HASH_SIZE 32

#define MAX_PLAINTEXT_SIZE 32

byte cbc_iv[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

const int firstButtonPin = 4;     // the number of the pushbutton pin
const int secondButtonPin = 5;
//const int ledPin =  13;

int firstButtonPushCounter = 0;   // counter for the number of button presses
int firstButtonState = 0;         // current state of the button
int lastFirstButtonState = 0;     // previous state of the button

int secondButtonPushCounter = 0;   // counter for the number of button presses
int secondButtonState = 0;         // current state of the button
int lastSecondButtonState = 0;     // previous state of the button

SHA256 sha256;
CBC<AES256> cbcaes256;

String rem_msg = "";
String cur_msg = "";
int aes_key[MAX_PLAINTEXT_SIZE];
int key_index = 0;
int counter = 0;

boolean waitForPassword = false;
boolean waitForDecrypt = false;
boolean alreadyDone = false;
boolean initMessage = true;

void setup()
{
    pinMode(firstButtonPin, INPUT);
    pinMode(secondButtonPin, INPUT);
    
    Serial.begin(9600);  // Debugging only
    if (!driver.init())
         Serial.println("init failed");
}

void loop()
{
  if (initMessage) {
    initMessage = false;
    Serial.println("Arduino initialized");
  }
  msgReceiver();
//  decryptMsg();
  buttonsHandler();
}

void helpMessage() {
    Serial.println("Enter your password (push both buttons to start input)");
}

void msgReceiver() {
    uint8_t buf[128];
    uint8_t buflen = sizeof(buf);

    if (driver.recv(buf, &buflen)) // Non-blocking
    {
       int i;
       cur_msg = "";
       for (int i = 0; i < buflen; i++) {
            cur_msg += (char)buf[i];
        }
       
       Serial.println("New message arrived");
       if (!waitForPassword) {
          helpMessage();
        }
    }
  }

void decryptMsg() {
      Hash *hash = &sha256;
      
      char key[key_index];
      Serial.println("passw:");
      for (int i = 0; i < key_index; i++) {
        Serial.print(aes_key[i]);
        key[i] = aes_key[i];
      }

      Serial.println();
      
      size_t k_size = sizeof(key);  
      uint8_t hashKey[HASH_SIZE];
      hash->reset();
      hash->update(key + 0, k_size);
      hash->finalize(hashKey, sizeof(hashKey));
      if (rem_msg.length() > 0) {
          char tmpMsg[rem_msg.length()];

          for (int i = 0; i < rem_msg.length(); i++) {
              tmpMsg[i] = rem_msg[i];
            }
        
          decCBCMode((const char *)tmpMsg, rem_msg.length(), hashKey, sizeof(hashKey));
          rem_msg = "";
          waitForPassword = false;
        }      
  }

void decCBCMode(const char *msg, size_t msg_len, byte *hashKey, size_t hashKeySize) {
    Cipher *cipher = &cbcaes256;    
    byte *myhashKey = hashKey;
    size_t m_size = msg_len;
    if (m_size % 16 != 0) {
        Serial.println("Wrong message was received.");
        return;
      }
    
    uint8_t *cur_msg = (uint8_t *) msg;
    byte output[m_size];
    cipher->clear();

    if (!cipher->setKey(myhashKey, cipher->keySize())) {
        Serial.print("setKey ");
        return;
      }

    if (!cipher->setIV(cbc_iv, cipher->ivSize())) {
      Serial.print("setIV ");
      return;
    }

    memset(output, 0xBA, sizeof(output));

    cipher->decrypt(output+0, cur_msg+0, m_size);
    int skip_letters = output[m_size - 1];
    if (skip_letters == 0) {
        skip_letters = 16;
      }
    String res_msg = "";
    for (int i = 0; i < sizeof(output) - skip_letters; i++) {
        res_msg += (char)output[i];
      }
      Serial.println("Decrypted msg: ");
      Serial.println(res_msg);
  }

void buttonsHandler() {
    firstButtonState = digitalRead(firstButtonPin);
    secondButtonState = digitalRead(secondButtonPin);
    
    if (firstButtonState != lastFirstButtonState) {
      // if the state has changed, increment the counter
      if (firstButtonState == HIGH) {
        // if the current state is HIGH then the button went from off to on:
        firstButtonPushCounter++;
      } else {
         if (secondButtonState == HIGH) {
            if (!alreadyDone) {
              alreadyDone = true;
              // both buttons are pressed
              if (waitForPassword) {
                decryptMsg();
              } else {
                if (cur_msg.length()) {
                  rem_msg = cur_msg;
                  cur_msg = "";
                  memset(aes_key, 0, sizeof MAX_PLAINTEXT_SIZE);
                  key_index = 0;
                  
                  Serial.println("One message available. Enter your password");  
                  Serial.println("(push both buttons to confirm the password and decrypt)");
                  waitForPassword = true; 
                  
                } else {
                  Serial.println("No messages available");  
                }
              }
            }
          } else {
            // only one is pressed
            if (waitForPassword) { 
              if (alreadyDone) {
                alreadyDone = false;
              } else {
                if (key_index < MAX_PLAINTEXT_SIZE) { 
                  aes_key[key_index] = firstButtonPin;
                  key_index += 1;
                }
              }
            }
          }
      }
      // Delay a little bit to avoid bouncing
      delay(50);
    }
    // save the current state as the last state, for next time through the loop
    lastFirstButtonState = firstButtonState;
    
    if (secondButtonState != lastSecondButtonState) {
      // if the state has changed, increment the counter
      if (secondButtonState == HIGH) {
        // if the current state is HIGH then the button went from off to on:
        secondButtonPushCounter++;
      } else {
          if (firstButtonState == HIGH) {
            if (!alreadyDone) { 
              alreadyDone = true;
            // both buttons are pressed
              if (waitForPassword) {
                decryptMsg();
              } else {
                if (cur_msg.length()) {
                  Serial.println("One message available. Enter your password");  
                  Serial.println("(push both buttons to confirm the password and decrypt)");
                  memset(aes_key, 0, sizeof MAX_PLAINTEXT_SIZE);
                  key_index = 0;
                  rem_msg = cur_msg;
                  cur_msg = "";
                  waitForPassword = true; 
                } else {
                  Serial.println("No messages available");  
                }
              }
            }
          } else {
            if (waitForPassword) { 
              // only one is pressed
              if (alreadyDone) {
                alreadyDone = false;
              } else {
                if (key_index < MAX_PLAINTEXT_SIZE) { 
                  aes_key[key_index] = secondButtonPin;
                  key_index += 1;
                }
              }
            }
          }
      }
      // Delay a little bit to avoid bouncing
      delay(50);
    }
    // save the current state as the last state, for next time through the loop
    lastSecondButtonState = secondButtonState;
    
}
