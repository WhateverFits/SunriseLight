#include <ArduinoJson.h>

#include <TimeAlarms.h>

#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <Timezone.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Sunrise.h"


#define DEBOUNCE 50
#define REPEATDELAY 1000
#define DELAY 1000
#define LEDS 60
#define localport 8888
#define NEO_PIN D6
#define BUTTON_PIN D2

//const char* ssid = "The Ranch-2.4";  //  your network SSID (name)
//const char* pass = "916-955-0942";       // your network password
//const char* ssid = "ThunderLizard";  //  your network SSID (name)
//const char* pass = "SnarfSnarf";       // your network password
const char* ssid = "Acrid";  //  your network SSID (name)
const char* pass = "MyVoiceIsMyPassport";       // your network password
//const char* ssid = "MakerHQ";  //  your network SSID (name)
//const char* pass = "sacramentomaker916";       // your network password
IPAddress timeServer(192, 168, 80, 1);
//IPAddress timeServer(63, 224, 11, 139);
const char* ntpServerName = "pool.ntp.org";
TimeChangeRule myDST = {"PDT", Second, Sun, Mar, 2, -420};    //Daylight time = UTC - 7 hours
TimeChangeRule mySTD = {"PST", First, Sun, Nov, 2, -480};     //Standard time = UTC - 8 hours
Timezone myTZ(myDST, mySTD);


Sunrise sunrise = Sunrise(DELAY, LEDS, NEO_PIN);
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets


unsigned long lastButtonTime[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned long lastButtonState[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned long buttonState[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

bool debounceButton(int pin) {
  bool result = false;
  unsigned long now = millis();
  int reading = digitalRead(pin);
  if (reading != buttonState[pin])
    lastButtonTime[pin] = now;

  if (abs(now - lastButtonTime[pin]) > DEBOUNCE) {
    if (reading != buttonState[pin])
      buttonState[pin] = reading;
    if (buttonState[pin] == LOW) {
      if (now - lastButtonState[pin] > REPEATDELAY) {
        Serial.print("Button: ");
        Serial.println(pin);
        result = true;
        lastButtonState[pin] = now;
      }
    }
  }

  return result;
}

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev
void MorningAlarm() {
  Serial.println("Good morning!");
  time_t utc = now();
  time_t local = myTZ.toLocal(utc, &tcr);
  printTime(local, tcr -> abbrev);
  sunrise.StartSunrise();
}

void EveningAlarm() {
  Serial.println("Good evening!");
  time_t utc = now();
  time_t local = myTZ.toLocal(utc, &tcr);
  printTime(local, tcr -> abbrev);
  sunrise.StartSunset();
}

void MoonAlarm() {
  Serial.println("Watch out for wherewolves!");
  time_t utc = now();
  time_t local = myTZ.toLocal(utc, &tcr);
  printTime(local, tcr -> abbrev);
  sunrise.StartMoonrise();
}

void MoonSetAlarm() {
  Serial.println("Whew!");
  time_t utc = now();
  time_t local = myTZ.toLocal(utc, &tcr);
  printTime(local, tcr -> abbrev);

  sunrise.StartMoonset();
}

long lastTime = 0;
time_t utc, local;

bool alarmsSet = false;
int morningIndex = -1;
int eveningIndex = -1;

int createAlarmUTC(int h, int m, OnTick_t onTickHandler) {
  TimeElements t;
  t.Second = 0;
  t.Minute = m;
  t.Hour = h;
  t.Day = 18;
  t.Month = 11;
  t.Year = 2017 - 1970;
  time_t utc = makeTime(t);
  return Alarm.alarmRepeat(hour(utc), m, 0, onTickHandler);
}

int createAlarm(int h, int m, OnTick_t onTickHandler) {
  TimeElements t;
  t.Second = 0;
  t.Minute = m;
  t.Hour = h;
  t.Day = 18;
  t.Month = 11;
  t.Year = 2017 - 1970;
  time_t localTime = makeTime(t);
  time_t utc = myTZ.toUTC(localTime);
  return Alarm.alarmRepeat(hour(utc), m, 0, onTickHandler);
}

void setupAlarms() {
  if (!alarmsSet) {
    for (int i = 0; i < 100; i++) {
      Alarm.free(i);
    }

    getSunriseSunsetTimes();
    createAlarm(22, 0, MoonAlarm);
    createAlarm(23, 0, MoonSetAlarm);
    alarmsSet = true;
  }
}

/*void loop3() {
  static long last = 0;
  static int i = 0;
  if (abs(millis() - last) > 200) {
    last = millis();
    strip.setPixelColor(i, 50, 50, 50); // Draw new pixel
    strip.setPixelColor(i - 5, 0, 0, 0); // Erase pixel a few steps back
    strip.show();
    i++;
    if (i > 205)
      i = 0;
  }
  }*/

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  WiFi.hostByName(ntpServerName, timeServer);
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
      return secsSince1900 - 2208988800UL;// + timeZone * SECS_PER_HOUR;
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

void printTime(time_t t, char *tz)
{
  sPrintI00(hour(t));
  sPrintDigits(minute(t));
  sPrintDigits(second(t));
  Serial.print(' ');
  Serial.print(dayShortStr(weekday(t)));
  Serial.print(' ');
  sPrintI00(day(t));
  Serial.print(' ');
  Serial.print(monthShortStr(month(t)));
  Serial.print(' ');
  Serial.print(year(t));
  Serial.print(' ');
  Serial.print(tz);
  Serial.println();
}

//Print an integer in "00" format (with leading zero).
//Input value assumed to be between 0 and 99.
void sPrintI00(int val)
{
  if (val < 10) Serial.print('0');
  Serial.print(val, DEC);
  return;
}

//Print an integer in ":00" format (with leading zero).
//Input value assumed to be between 0 and 99.
void sPrintDigits(int val)
{
  Serial.print(':');
  if (val < 10) Serial.print('0');
  Serial.print(val, DEC);
}

void getSunriseSunsetTimes() {
  WiFiClient client;
  const int httpPort = 80;
  const char* host = "api.sunrise-sunset.org";
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // We now create a URI for the request
  String url = "/json?lat=38.581572&lng=-121.494400&formatted=0";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  while (client.available()) {
    String line = client.readStringUntil('\r');
    line = line.substring(1);
    if (line[0] == '{') {
      StaticJsonBuffer<1000> jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(line.substring(line.indexOf(':') + 1, line.lastIndexOf(',')));
      String sunrise = root.get<String>("sunrise");
      String sunset = root.get<String>("sunset");

      Serial.print("Sunrise: ");

      String sunriseHour = sunrise.substring(sunrise.indexOf('T') + 1, sunrise.indexOf(':'));
      String sunriseMin = sunrise.substring(sunrise.indexOf(':') + 1, sunrise.indexOf(':') + 3);
      String sunsetHour = sunset.substring(sunset.indexOf('T') + 1, sunset.indexOf(':'));
      String sunsetMin = sunset.substring(sunset.indexOf(':') + 1, sunset.indexOf(':') + 3);
      if (morningIndex > -1)
        Alarm.free(morningIndex);
      if (eveningIndex > -1)
        Alarm.free(eveningIndex);
      morningIndex = createAlarmUTC(sunriseHour.toInt(), sunriseMin.toInt(), MorningAlarm);
      eveningIndex = createAlarmUTC(sunsetHour.toInt(), sunsetMin.toInt(), EveningAlarm);
      break;
    }
  }

  Serial.println();
  Serial.println("closing connection");
}

void setup() {
  //Serial.begin(74880);
  Serial.begin(115200);
  delay(100);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  pinMode(BUTTON_PIN, INPUT_PULLUP);


  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("NEO_PIN: ");
  Serial.println(NEO_PIN);
  Serial.print("BUTTON: ");
  Serial.println(BUTTON_PIN);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Netmask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());


  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
}

void loop() {
  Alarm.delay(0);
  sunrise.Update();
  static bool bSunrise = false;
  static bool bSunset = false;
  static bool bMoonrise = false;

  if (debounceButton(BUTTON_PIN)) {
    if (!bSunrise && !bSunset && !bMoonrise) {
      bSunrise = true;
      Serial.println("Sunrise");
      sunrise.StartSunrise();
    } else if (bSunrise && !bSunset && !bMoonrise) {
      Serial.println("Sunset");
      bSunrise = false;
      bSunset = true;
      sunrise.StartSunset();
    } else if (!bSunrise && bSunset && !bMoonrise) {
      Serial.println("Moonrise");
      bSunrise = false;
      bSunset = false;
      bMoonrise = true;
      sunrise.StartMoonrise();
    } else {
      Serial.println("Moonset");
      bSunrise = false;
      bSunset = false;
      bMoonrise = false;
      sunrise.StartMoonset();
    }
  }

  long milliseconds = millis();
  if (milliseconds >= lastTime + 30000) {
    utc = now();
    local = myTZ.toLocal(utc, &tcr);
    printTime(local, tcr -> abbrev);
    lastTime = milliseconds;
    setupAlarms();
  }

}


