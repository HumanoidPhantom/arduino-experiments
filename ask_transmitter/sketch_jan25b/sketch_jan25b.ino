#include <blockcipher_descriptor.h>
#include <debug.h>
#include <hashfunction_descriptor.h>
#include <keysize_descriptor.h>
#include <sha3-api.h>
#include <stack_measuring.h>
#include <streamcipher_descriptor.h>
#include <string-extras.h>
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

#define MAX_PLAINTEXT_SIZE 64
#define MAX_CIHPERTEXT_SIZE 64

//const int ledPin =  13;
int firstButtonPushCounter = 0;   // counter for the number of button presses
int firstButtonState = 0;         // current state of the button
int lastFirstButtonState = 0;     // previous state of the button

int secondButtonPushCounter = 0;   // counter for the number of button presses
int secondButtonState = 0;         // current state of the button
int lastSecondButtonState = 0;     // previous state of the button

boolean showHelpMessage = true;
byte cbc_iv[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

SHA256 sha256;
CBC<AES256> cbcaes256;
char *aes_key = (char *)"this is test";
int counter = 0;

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
//    buttonsHandler();
}

void helpMessage() {
    if (showHelpMessage) {
        Serial.println("Enter your message: ");
        showHelpMessage = false;  
    }
  }

void recvWithEndMarker() {
//    if (Serial.available() > 0) {
//        String strMsg = Serial.readString();
//        int msgLen = strMsg.length();
//        char msg[msgLen];
//        int i;
//        for (i = 0; i < msgLen; i++) {
//               msg[i] = strMsg[i];
//          }
//        msg[i] = '\0';
//        showNewData(msg);
//  }
  char charCounter[16];
  counter +=1;
  itoa(counter, charCounter, 10);
  const char *msg = (const char *) "Nice to meet you Nice to meet you Nice to meet you Nice to meet you Nice to meet youNice to meet you";
  char newMsg[strlen(msg) + strlen(charCounter)];
  
  size_t msg_len = getMsg(msg, charCounter, newMsg, strlen(msg), strlen(charCounter));
  showNewData(newMsg, msg_len);
  delay(1000);
}

size_t getMsg(const char * msg, char * charCounter, char * newMsg, size_t msg_len, size_t counter_len) {        
    for (int i = 0; i < counter_len; i++) {
        newMsg[i] = charCounter[i];
      }
    
    for (int i = 0; i < msg_len; i++) {
        newMsg[counter_len + i] = msg[i];
      }

    newMsg[msg_len + counter_len - 1] = '\0';
    return msg_len + counter_len;
}


void showNewData(char *msg, size_t msg_len) {
     Serial.print("You sent message: ");
     Serial.println(msg);
     encryptMsg(msg, msg_len);
     showHelpMessage = true;
  }

void sendMessage(char *msg, size_t msg_len) {
    Serial.println(msg_len);
    driver.send((uint8_t *)msg, msg_len);
    driver.waitPacketSent();
  }

void encryptMsg(const char *msg, size_t msg_len) {
      Hash *hash = &sha256;
      const char *key = aes_key;
      size_t k_size = strlen(aes_key);
      size_t posn;
      uint8_t hashKey[HASH_SIZE];
      hash->reset();
      posn = 0;
      hash->update(key + posn, k_size);
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
        Serial.println("on");
        Serial.print("number of button pushes: ");
        Serial.println(firstButtonPushCounter);
      } else {
        // if the current state is LOW then the button went from on to off:
        Serial.println("off");
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
        Serial.println("second button on");
        Serial.print("number of second button pushes: ");
        Serial.println(secondButtonPushCounter);
      } else {
        // if the current state is LOW then the button went from on to off:
        Serial.println("second buttton off");
      }
      // Delay a little bit to avoid bouncing
      delay(50);
    }
    // save the current state as the last state, for next time through the loop
    lastSecondButtonState = secondButtonState;
}
