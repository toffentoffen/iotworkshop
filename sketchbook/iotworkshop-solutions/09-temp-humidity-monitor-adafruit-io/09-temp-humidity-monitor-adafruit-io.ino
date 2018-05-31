#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WEMOS_SHT3X.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <WorkShop_Logos.h>


// Configre Wifi Connection
const char* ssid     = "";
const char* password = "";
bool WIFI_LOGO_ON = true;

// Configure OLED display
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

// Variables needed for NTP
static const char ntpServerName[] = "us.pool.ntp.org";
const int timeZone = 0;  // UTC
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t getNtpTime();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);
time_t start_time;

// Configure STH30 sensor I2C address
SHT3X sht30(0x45);

// Configure sensor metrics destination
const String ADAFRUIT_USERNMAE = "";
const String ADAFRUIT_FEED_GROUP = "";
const String ADAFRUIT_AIO_HEADER_NAME = "";
const String ADAFRUIT_AIO_HEADER_VALUE = "";
const String SENSOR_METRICS_DESTINATION = "http://io.adafruit.com/api/v2/" +  ADAFRUIT_USERNMAE + "/groups/" + ADAFRUIT_FEED_GROUP + "/data";

void setup() {

  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  drawOracleLogo(display);
  drawNoWifiLogo(display);
  
  connectToWifi(ssid, password);

  syncTimeFromNTP();
  start_time = now();

}

void loop() {
  
  clearDisplayBuffer();

  if(sht30.get()==0){
    printSensorsMetricsToOLEDDisplay();
    
    if (WiFi.status() == WL_CONNECTED) {
      sendSensorData(sht30.cTemp, sht30.humidity);
      timeStatus_t status = timeStatus();
      if (status != timeSet){
        Serial.println("Failed to sync time: " + String(status));
      }
    }else {
      Serial.println("Ay caramba !!!!!!! Not connected to the Wifi");
    }
  }
  else
  {
    display.println("Error trying to get STH30 values!");
    drawNoWifiLogoOff(display);
  }
  
  display.display();
  delay(1000);

}

void connectToWifi(const char* ssid, const char* password) {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  display.setCursor(0, 60);
  display.setTextColor(WHITE);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    
    WIFI_LOGO_ON = drawWifiLogoToggle(display, WIFI_LOGO_ON);
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
}

void syncTimeFromNTP() {
  Serial.println("Setting up NTP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
}

void clearDisplayBuffer() {
  // Clear the buffer.
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
}

void printSensorsMetricsToOLEDDisplay() {
  display.setTextSize(1);
  display.println("T: ");
  display.setTextSize(2);
  display.println(sht30.cTemp);

  display.setTextSize(1);
  display.println("H: ");
  display.setTextSize(1);
  display.println(sht30.humidity);
 
  if (timeStatus() != timeSet)
  {
    display.setTextSize(1);
    display.print("Not sending!!!");
    display.setTextSize(1);
  }
  
}

String getCurrentTimestamp() {
  String timestamp;
  String run_millis;
  int millis_chars;
  
  run_millis = String(millis());
  millis_chars = run_millis.length();
  // To generate a millisecond unix timestamp, we first get the second timestamp, and add to it, the last three characters of the arduino/relative millisecond timestamp
  timestamp = String(now()) + run_millis.charAt(millis_chars-3) + run_millis.charAt(millis_chars-2) + run_millis.charAt(millis_chars-1);

  return timestamp;
}

void sendSensorData(float temp, float humidity) {
  HTTPClient http;    //Declare object of class HTTPClient
  
  http.begin(SENSOR_METRICS_DESTINATION);               // Specify request destination
  http.addHeader(ADAFRUIT_AIO_HEADER_NAME, ADAFRUIT_AIO_HEADER_VALUE);    // Specify Adafruit aio key header
  http.addHeader("Content-Type", "application/json");    // Specify content-type header
  
  String sensorPayload = String("{\n") + 
    String("\"feeds\":[\n") + 
      String("{\n") + 
        String("\"key\": \"temperature\",\n") + 
        String("\"value\": \"") +  String(temp) + 
        String("\"\n") + 
        String("},{\n") + 
        String("\"key\": \"humidity\",\n") + 
        String("\"value\": \"") +  String(humidity) + 
        String("\"\n") + 
      String("}\n") + 
    String("]\n") + 
  String("}");

  Serial.println("Sending sensor metrics to " + SENSOR_METRICS_DESTINATION + ":\n" + sensorPayload); // Print payload
  
  int httpCode = http.POST(sensorPayload);      //Send the request
  String payload = http.getString();
  Serial.println(payload);
  if (httpCode < 0) {
    Serial.println("Error sending sensor measures:" + String(httpCode));
  } else {
    Serial.println("Http response [" + String(httpCode) + "]\n");    //Print request response     
    if(httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println(payload);
    }
  }
  
  http.end();  //Close connection
}

/*----------------------------*/
/*-------- NTP code ----------*/
/*----------------------------*/
/* Copied from https://github.com/PaulStoffregen/Time/blob/master/examples/TimeNTP_ESP8266WiFi/TimeNTP_ESP8266WiFi.ino#L99 */
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}
// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
