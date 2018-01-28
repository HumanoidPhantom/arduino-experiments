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

#define MAX_PLAINTEXT_SIZE 128

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

String prev_msg = "";
String cur_msg = "";
char *aes_key = (char *)"this is test";

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
    msgReceiver();
    decryptMsg();
    buttonsHandler();
}

void msgReceiver() {
    uint8_t buf[MAX_PLAINTEXT_SIZE];
    uint8_t buflen = sizeof(buf);

    if (driver.recv(buf, &buflen)) // Non-blocking
    {
       int i;
       String str = "";
       for (int i = 0; i < buflen; i++) {
            str += (char)buf[i];
        }
       prev_msg = cur_msg;
       cur_msg = str;
       Serial.println("New message arrived");
    }
  }

void decryptMsg() {
      Hash *hash = &sha256;
      const char *key = (const char *) aes_key;
      size_t k_size = strlen(key);  
      uint8_t hashKey[HASH_SIZE];
      hash->reset();
      hash->update(key + 0, k_size);
      hash->finalize(hashKey, sizeof(hashKey));
      
      if (prev_msg.length() > 0) {
          Serial.println("got here 1");
          char tmpMsg[prev_msg.length()];

          for (int i = 0; i < prev_msg.length(); i++) {
              tmpMsg[i] = prev_msg[i];
            }
        
          decCBCMode((const char *)tmpMsg, prev_msg.length(), hashKey, sizeof(hashKey));
          prev_msg = "";
        }

      if (cur_msg.length() > 0) {
        Serial.print("got here 2, "); Serial.println(cur_msg.length());
          char tmpMsg[cur_msg.length()];

          for (int i = 0; i < cur_msg.length(); i++) {
              tmpMsg[i] = cur_msg[i];
            }
        
        decCBCMode((const char *)tmpMsg, cur_msg.length(), hashKey, sizeof(hashKey));
        cur_msg = "";
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
      Serial.println("done");

      delay(1000);

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
