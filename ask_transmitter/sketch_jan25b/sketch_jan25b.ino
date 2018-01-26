#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile

RH_ASK driver(2000, 9, 2, 10); // ESP8266 or ESP32: do not use pin 11

const int firstButtonPin = 4;     // the number of the pushbutton pin
const int secondButtonPin = 5;

//const byte msgLen = 200;
//char msg[msgLen];

//const int ledPin =  13;
int firstButtonPushCounter = 0;   // counter for the number of button presses
int firstButtonState = 0;         // current state of the button
int lastFirstButtonState = 0;     // previous state of the button

int secondButtonPushCounter = 0;   // counter for the number of button presses
int secondButtonState = 0;         // current state of the button
int lastSecondButtonState = 0;     // previous state of the button

boolean showHelpMessage = true;

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
    if (showHelpMessage) {
        Serial.println("Enter your message: ");
        showHelpMessage = false;  
    }
  }

void recvWithEndMarker() {
    if (Serial.available() > 0) {
        String strMsg = Serial.readString();
        int msgLen = strMsg.length();
        char msg[msgLen];
        int i;
        for (i = 0; i < msgLen; i++) {
               msg[i] = strMsg[i];
          }
        msg[i] = '\0';
        showNewData(msg);
  }
}

void showNewData(char *msg) {
      sendMessage(msg);
      Serial.print("You sent message: ");
      Serial.println(msg);
      showHelpMessage = true;
  }

void sendMessage(char *msg) {
    driver.send((uint8_t *)msg, strlen(msg));
    driver.waitPacketSent();
  }

void encryptMsg(char *msg) {
      char *encKey = "this is my key";
      
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
