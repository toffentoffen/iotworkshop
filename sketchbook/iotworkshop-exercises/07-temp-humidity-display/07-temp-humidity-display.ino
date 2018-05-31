#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WEMOS_SHT3X.h>
#include <WorkShop_Logos.h>

// Configure OLED display
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);
// Configure STH30 sensor I2C address
SHT3X sht30(0x45);

void setup() {

  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

}

void loop() {
  
  clearDisplayBuffer();

  if(sht30.get()==0){
    printSensorsMetricsToOLEDDisplay();
  }
  else
  {
    display.println("Error trying to get STH30 values!");
  }
  
  display.display();
  delay(1000);

}

void clearDisplayBuffer() {
  // Clear the buffer.
}

void printSensorsMetricsToOLEDDisplay() {

}
