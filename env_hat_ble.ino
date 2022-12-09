#include <M5StickCPlus.h>
#include <Wire.h>
#include <PubSubClient.h> //library used to publishing and subscribinng topics
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>


// The remote service we wish to connect to.
#define CHARACTERISTIC_UUID "8ac406f9-584e-435f-b0dc-613e83c84361"
// The characteristic of the remote service we are interested in.
#define SERVICE_UUID "b952a134-a33b-4a39-bc07-cd88c1bf3ef0"


const char* ssid = "anant";

const char* password = "12345678";
const char* mqtt_server = "172.20.10.3";

WiFiClient espClient; //since we are using wifi we have provided wificlient object
PubSubClient * client;

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLECharacteristic *pCharacteristic;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;



float cTemp    = 0;
float fTemp    = 0;
float humidity = 0;
bool servo = false;
#define MSG_BUFFER_SIZE (500)
char msg1[MSG_BUFFER_SIZE];
char msg2[MSG_BUFFER_SIZE];

void setup() {
  M5.begin();
  Wire.begin(0,26);
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 4);
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  BLEDevice::init("Temperature_Requester");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setValue("Requesting from temp stick");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
  unsigned int data[6];
  // Start I2C Transmission
  Wire.beginTransmission(0x44);
  // Send measurement command
  Wire.write(0x2C);
  Wire.write(0x06);
  // Stop I2C transmission
  if (Wire.endTransmission() != 0){
    Serial.println("ERROR -- endTransmission Error!");
  }
  else{
    //M5.Lcd.print(" - First if done - ");
    delay(500);
    // Request 6 bytes of data
    Wire.requestFrom(0x44, 6);

    // Read 6 bytes of data
    // cTemp msb, cTemp lsb, cTemp crc, humidity msb, humidity lsb, humidity crc
    for (int i = 0; i < 6; i++) {
      data[i] = Wire.read();
    };
    delay(50);

    if (Wire.available() != 0){
      Serial.println("ERROR -- Wire.available still has data!");
    }
    else{
      //M5.Lcd.print(" - second if done - ");
      // Convert the data
      cTemp    = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45;
      fTemp    = (cTemp * 1.8) + 32;
      humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);
      Serial.print("celsius: ");
      Serial.println(cTemp);
      M5.Lcd.setCursor(0, 30, 2);
      //M5.Lcd.printf("Temp: %2.1f Humi: %2.0f", cTemp, humidity);
      if (cTemp < 30){
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.printf("Current temp is : %2.1f \n",cTemp);  
      M5.Lcd.printf("Temperature is within desired range. Happy times");
      pCharacteristic->setValue("GOOD");
      }
      else{
        //M5.Lcd.print(" - third if done - ");
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.printf("Current temp is : %2.1f \n",cTemp);
        M5.Lcd.printf("Temperature is higher than normal. Take ACTION");
        M5.update();  // Read the press state of the key.
        delay(5000);
        if (M5.BtnA.wasReleased()) {
            M5.Lcd.fillScreen(BLACK);
            M5.Lcd.printf("Alert Acknowledged. Happy Times again");
            pCharacteristic->setValue("GOOD");
        }
        else{
          M5.Lcd.fillScreen(BLACK);
          M5.Lcd.setCursor(0, 0, 4);
          M5.Lcd.printf("Current temp is : %2.1f \n",cTemp);
          M5.Lcd.print("Requesting for servo sprinkler to lower down temp");
          pCharacteristic->setValue("ALERT");
          delay(5000);
          //setup_ble();

        }
        }
    }
    }
    delay(2000);
  }

