#include <TimeAlarms.h>

#include <Time.h>
#include <TimeLib.h> 
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include "Sunrise.h"


#define DEBOUNCE 150
#define DELAY 1000
#define LEDS 200
#define localport 8888
const char ssid[] = "*************";  //  your network SSID (name)
const char pass[] = "********";       // your network password
IPAddress timeServer(192, 168, 80, 1);
const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDS, 7, NEO_GRB + NEO_KHZ800);
Sunrise sunrise(DELAY, strip, LEDS);
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Setup start");

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  
  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
}


unsigned long lastButton[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
bool debounceButton(int pin) {
  unsigned long now = millis();
  if (digitalRead(pin) == LOW && abs(now - lastButton[pin]) > DEBOUNCE) {
    lastButton[pin] = now;
    Serial.print("Button: ");
    Serial.println(pin);
    return true;
  } else {
    return false;
  }
}

void loop() {
  sunrise.Update();
  static bool bSunrise = true;
  static bool bSunset = false;

  if (debounceButton(0)){
    if (!bSunrise && !bSunset) {
      bSunrise = true;
      sunrise.StartSunrise();
    } else if (bSunrise && !bSunset) {
      bSunrise = false;
      bSunset = true;
      sunrise.StartSunset();
    } else if (!bSunrise && bSunset) {
      bSunrise = true;
      bSunset = false;
    }
  }

}

void loop3() {
  static long last = 0;
  static int i = 0;
  if (abs(millis() - last) > 200) {
    last = millis();
    strip.setPixelColor(i, 50,50,50); // Draw new pixel
    strip.setPixelColor(i-5, 0,0,0); // Erase pixel a few steps back
    strip.show();
    i++;
    if (i > 205)
      i = 0;
  }
}
void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year()); 
  Serial.println(); 
}



void printDigits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
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
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
