#include <RH_ASK.h>
#include <SPI.h> // Not actualy used but needed to compile

RH_ASK driver(2000, 2, 4, 10); // ESP8266 or ESP32: do not use pin 11 


#include <rfscomm.h>

/***************************************************************************************/

// I/O defines
#define P_RX       2 // RF Rx digital pin
//#define P_LED     13 // LED pin

// RF defines
#define MY_ID   0x1B // Decimal value:  27 - One Byte, range (0-255) | (0x00-0xFF)

/***************************************************************************************/

// RF Object
RFSCOMM rf(MY_ID);

void setup()
{
    Serial.begin(9600);  // Debugging only
    if (!driver.init())
         Serial.println("init failed");


    rf.config(NO_USE, P_RX, MAN_1200);

    Serial.print("\nSystem with ID 0x"); Serial.print(MY_ID, HEX); Serial.print(", configured to receive\n");
    Serial.print("\n-------------------------------------------------------\n");
}

void loop()
{
    uint8_t src_id, dev_id, data_length;
    uint8_t* data;
//    uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
//    uint8_t buflen = sizeof(buf);

//    if (driver.recv(buf, &buflen)) // Non-blocking
//    {
//       int i;
//       String str = "";
//       for (int i = 0; i < buflen; i++) {
//            str += (char)buf[i];
//        }
//       
//        Serial.println(str);
//    }

        // If any packet received
    if(rf.receive())
    {
        // Get the packet content
        src_id = rf.rx_src_id();
        dev_id = rf.rx_dev_id();
        data_length = rf.rx_data_length();
        data = rf.rx_data();

        // Notify and show packet content
        Serial.print("\nPackage received:\n");
        Serial.print("[From device with ID: 0x"); Serial.print(src_id, HEX); Serial.print(" | ");
        Serial.print("To device with ID: 0x"); Serial.print(dev_id, HEX); Serial.print(" | ");
        Serial.print("Data ("); Serial.print(data_length); Serial.print(" bytes): ");
        Serial.print((const char*)data); Serial.print("]\n\n");

        // Check if the packet is for me
        if(dev_id == MY_ID)
        {
            Serial.print("Expected data from device [0x"); Serial.print(src_id, HEX); Serial.print("] has been received.\n");
            Serial.print("Data received: "); Serial.print((const char*)data); Serial.println();
            Serial.print("\n-------------------------------------------------------\n");
        }
    }
}
