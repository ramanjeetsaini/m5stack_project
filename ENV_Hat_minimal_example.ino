#include <M5StickCPlus.h>
#include <Wire.h>

float cTemp    = 0;
float fTemp    = 0;
float humidity = 0;

void setup() {
  M5.begin();
  Wire.begin(0,26);
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 4);
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
      Serial.println(cTemp);
      M5.Lcd.setCursor(0, 30, 2);
      M5.Lcd.printf("Temp: %2.1f Humi: %2.0f", cTemp, humidity);
    }
  }
  delay(2000);
}