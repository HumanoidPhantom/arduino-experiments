#include <AES.h>
#include <CBC.h>
#include <Crypto.h>
#include <string.h>
#include <SHA256.h>

#define HASH_SIZE 32

#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile

RH_ASK driver(2000, 9, 2, 10); // ESP8266 or ESP32: do not use pin 11

const int firstButtonPin = 4;     // the number of the pushbutton pin
const int secondButtonPin = 5;

#define MAX_PLAINTEXT_SIZE 32

//const int ledPin =  13;
int firstButtonPushCounter = 0;   // counter for the number of button presses
int firstButtonState = 0;         // current state of the button
int lastFirstButtonState = 0;     // previous state of the button

int secondButtonPushCounter = 0;   // counter for the number of button presses
int secondButtonState = 0;         // current state of the button
int lastSecondButtonState = 0;     // previous state of the button

boolean showHelpMessage = true;
boolean waitForPassword = false;

byte cbc_iv[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

SHA256 sha256;
CBC<AES256> cbcaes256;
int aes_key[MAX_PLAINTEXT_SIZE];
int key_index = 0;
int counter = 0;

char send_msg[MAX_PLAINTEXT_SIZE];

void setup()
{
    pinMode(firstButtonPin, INPUT);
    pinMode(secondButtonPin, INPUT);

    
    Serial.begin(9600);    // Debugging only
    if (!driver.init())
         Serial.println("init failed");
}

void loop()
{  
    helpMessage();
    recvWithEndMarker();
    buttonsHandler();
}

void helpMessage() {
    if (showHelpMessage && !waitForPassword) {
        Serial.println("Enter your message: ");
        showHelpMessage = false;  
    }
  }

void recvWithEndMarker() {
    if (Serial.available() > 0 && !waitForPassword) {
        memset(send_msg, 0, sizeof MAX_PLAINTEXT_SIZE);
        memset(aes_key, 0, sizeof MAX_PLAINTEXT_SIZE);
        key_index = 0;
        String strMsg = Serial.readString();
        int msgLen = strMsg.length();
//        char msg[msgLen];
        int i;
        for (i = 0; i < msgLen && i < MAX_PLAINTEXT_SIZE; i++) {
               send_msg[i] = strMsg[i];
          }
        send_msg[i] = '\0';
        Serial.print("Your message: ");
        Serial.println(send_msg);
        Serial.println("Enter your password (press both buttons to confirm and send): ");
        waitForPassword = true;
//        showNewData(msg, strlen(msg));
  }
//  char charCounter[16];
//  counter +=1;
//  itoa(counter, charCounter, 10);
//  const char *msg = (const char *) "Nice to meet youNice to meet";
  
//  size_t msg_len = strlen(msg) + strlen(charCounter);
//  char newMsg[strlen(msg) + strlen(charCounter)];

//  sprintf(newMsg, "%s%s", charCounter, msg);
//  showNewData(newMsg, msg_len);
//  delay(1000);
}

void encryptAndSend() {
  Serial.println("passw:");
  for (int i = 0; i < key_index; i++) {
    Serial.print(aes_key[i]);
  }
  Serial.println();
  waitForPassword = false;
  showNewData(send_msg, strlen(send_msg));
}

void showNewData(char *msg, size_t msg_len) {
     encryptMsg(msg, msg_len);
     showHelpMessage = true;
  }

void sendMessage(char *msg, size_t msg_len) {
    Serial.println("The message is sent");
    driver.send((uint8_t *)msg, msg_len);
    driver.waitPacketSent();
  }

void encryptMsg(const char *msg, size_t msg_len) {
      Hash *hash = &sha256;
      char key[key_index];
      for (int i = 0; i < key_index; i++) {
        key[i] = aes_key[i];
      }
      size_t k_size = sizeof(key);
      uint8_t hashKey[HASH_SIZE];
      hash->reset();
      hash->update(key + 0, k_size);
      hash->finalize(hashKey, sizeof(hashKey));

      encCBCMode(msg, msg_len, hashKey, sizeof(hashKey));
  }

void encCBCMode(const char *msg, size_t msg_len, byte *hashKey, size_t hashKeySize) {
    Cipher *cipher = &cbcaes256;    
    byte *myhashKey = hashKey;
    size_t m_real_size = msg_len;
    size_t append_block = 0;
    if (m_real_size % 16 == 0) {
        append_block = 16;
      } else {
        append_block = 16 - m_real_size % 16;  
      }
    size_t m_size = m_real_size + append_block;
    uint8_t cur_msg[m_size];
    byte output[m_size];
    for (int i = 0; i < m_size; i++) {
        if (i < m_real_size) {
            cur_msg[i] = msg[i];
          } else {
            if (append_block == 16) {
              cur_msg[i] = 0;  
            } else {
              cur_msg[i] = append_block;
            }
          }
      }

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

    cipher->encrypt(output+0, cur_msg+0, m_size);
    sendMessage((char *) output, sizeof(output));
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
        // if the current state is LOW then the button went from on to off:
        if (waitForPassword) {
           if (secondButtonState == HIGH) {
            encryptAndSend();
          } else {
            if (key_index < MAX_PLAINTEXT_SIZE) { 
              aes_key[key_index] = firstButtonPin;
              key_index += 1;  
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
        // if the current state is LOW then the button went from on to off:
        if (waitForPassword) {
            if (firstButtonState == HIGH) {
              encryptAndSend();
            } else {
                if (key_index < MAX_PLAINTEXT_SIZE) { 
                aes_key[key_index] = secondButtonPin;
                key_index += 1;
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
