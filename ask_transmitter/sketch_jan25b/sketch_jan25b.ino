#include <RH_ASK.h>
#include <SPI.h> // Not actualy used but needed to compile
#include <rfscomm.h>

// I/O defines
#define P_TX         2 // Tx digital pin

// RF defines
#define MY_ID     0xA5 // Decimal value: 165 - One Byte, range (0-255) | (0x00-0xFF)
#define DEVICE_ID 0x1B // Decimal value:  27 - One Byte, range (0-255) | (0x00-0xFF)
#define DATA      0x12 // Decimal value:  18 - One Byte, range (0-255) | (0x00-0xFF)

// RF Object
RFSCOMM rf(MY_ID);

RH_ASK driver(2000, 9, 2, 10); // ESP8266 or ESP32: do not use pin 11

void setup()
{
    Serial.begin(9600);  // Debugging only
    if (!driver.init())
         Serial.println("init failed");

    rf.config(P_TX, NO_USE, MAN_1200);
    
    Serial.print("\nSystem with ID 0x"); Serial.print(MY_ID, HEX); Serial.print(", configured to transmit\n");
    Serial.print("\n-------------------------------------------------------\n");
}

void loop()
{
    uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
    uint8_t buflen = sizeof(buf);

    rf.send(DEVICE_ID, DATA);
    Serial.print("\nPackage sent:\n");
    Serial.print("[My ID: 0x"); Serial.print(MY_ID, HEX); Serial.print(" | ");
    Serial.print("To device with ID: 0x"); Serial.print(DEVICE_ID, HEX); Serial.print(" | ");
    Serial.print("Data: 0x"); Serial.print(DATA, HEX); Serial.print("\n");
    Serial.print("\n-------------------------------------------------------\n");
    delay(1000);

//    driver.send((uint8_t *)msg, strlen(msg));
//    driver.waitPacketSent();
//    Serial.println("done");
}
