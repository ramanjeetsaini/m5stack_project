#include <M5StickCPlus.h>
#include "BLEDevice.h"
#include <Wire.h>
//#include "BLEScan.h"

// The remote service we wish to connect to.
static BLEUUID serviceUUID("b952a134-a33b-4a39-bc07-cd88c1bf3ef0");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("8ac406f9-584e-435f-b0dc-613e83c84361");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLECharacteristic *pCharacteristic;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;


float cTemp    = 0;
float fTemp    = 0;
float humidity = 0;
extern const unsigned char m5stack_startup_music[];

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    M5.Lcd.print("Notify callback for characteristic ");
    M5.Lcd.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    M5.Lcd.print(" of data length ");
    M5.Lcd.print(length);
    M5.Lcd.print("data: ");
    M5.Lcd.print((char*)pData);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    M5.Lcd.print("onDisconnect");
  }
};

bool connectToServer() {
    //M5.Lcd.print("Forming a connection to ");
    //M5.Lcd.print(myDevice->getAddress().toString().c_str());
    Serial.print("Forming a connection to ");
    Serial.print(myDevice->getAddress().toString().c_str());
    BLEClient*  pClient  = BLEDevice::createClient();
    M5.Lcd.print(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    //M5.Lcd.print(" - Connected to server");
    Serial.print("-- connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      M5.Lcd.print("Failed to find our service UUID: ");
      M5.Lcd.print(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    //M5.Lcd.print(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      M5.Lcd.print("Failed to find our characteristic UUID: ");
      M5.Lcd.print(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    //M5.Lcd.print(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      //M5.Lcd.print("The characteristic value was: ");
      M5.Lcd.print(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    delay(10000);
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      Serial.print("BLE Advertised Device found: ");
      Serial.print(advertisedDevice.toString().c_str());

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    }
  }
}; // MyAdvertisedDeviceCallbacks


void setup() {
  M5.begin();
  Wire.begin(0,26);
  BLEDevice::init("");
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 4);
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  M5.Lcd.print(doConnect);
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  delay(20000);
  //ledcSetup(ledChannel, freq, resolution);
  //ledcAttachPin(servo_pin, ledChannel);
  //ledcWrite(ledChannel, 256);  // 0°
} // End of setup.


// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    M5.Lcd.print(doConnect);
    //M5.Lcd.print("Inside loop if");
    if (connectToServer()) {
      M5.Lcd.print("We are now connected to Temp Stick Server.");
    } else {
      M5.Lcd.print("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      //M5.Lcd.print("The characteristic value was: ");
      //M5.Lcd.print(value.c_str());
      if(value =="ALERT"){
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0, 4);
        M5.Lcd.print("ALERT from Temp Stick. \n Turning on sprinkler for temp");
        //ledcWriteTone(ledChannel, 1250);
        delay(1000);
        //ledcWriteTone(ledChannel, 0);
        delay(1000);
      }

      if(value =="GOOD"){
        M5.Lcd.fillScreen(BLACK);
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
            // Convert the data
          cTemp    = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45;
          fTemp    = (cTemp * 1.8) + 32;
          humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);
          Serial.print("celsius: ");
          Serial.println(humidity);
          M5.Lcd.setCursor(0, 30, 2);
          //M5.Lcd.printf("Temp: %2.1f Humi: %2.0f", cTemp, humidity);
          if (humidity > 50){
          M5.Lcd.fillScreen(BLACK); 
          M5.Lcd.setCursor(0, 0, 4); 
          M5.Lcd.printf("Current humidity is : %2.1f \n", humidity);
          M5.Lcd.printf("Moisture is within desired range. Happy times");
          }
          else{
            M5.Lcd.fillScreen(BLACK);
            M5.Lcd.setCursor(0, 0, 4);
            M5.Lcd.printf("Current humidity is : %2.1f \n", humidity);
            M5.Lcd.printf("Moisture is lower than normal. Turning ON Servo");

      }
    }
  }
  delay(2000);
      }




    }
    String newValue = "Time since boot: " + String(millis()/1000);
    //M5.Lcd.print("Setting new characteristic value to \"" + newValue + "\"");
    
    // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  delay(10000); // Delay a second between loops.
} // End of loop
