#include <ArduinoJson.h>

#include <TimeAlarms.h>

#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <Timezone.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Config.h"
#include "Sunrise.h"
#include "NTP.h"
#include "WebServer.h"

#define DEBOUNCE 50
#define REPEATDELAY 1000
#define DELAY 1000
#define LEDS 60
#define localport 8888
#define NEO_PIN D6
#define BUTTON_PIN D2

TimeChangeRule myDST = {"PDT", Second, Sun, Mar, 2, -420};    //Daylight time = UTC - 7 hours
TimeChangeRule mySTD = {"PST", First, Sun, Nov, 2, -480};     //Standard time = UTC - 8 hours
Timezone myTZ(myDST, mySTD);


Sunrise sunrise = Sunrise(DELAY, LEDS, NEO_PIN);
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
  bool enabled;
  EEPROM.get(0, enabled);
  if (enabled) 
  {
    Serial.println("Good morning!");
    time_t utc = now();
    time_t local = myTZ.toLocal(utc, &tcr);
    printTime(local, tcr -> abbrev);
    sunrise.StartSunrise();
  }
}

void EveningAlarm() {
  bool enabled;
  EEPROM.get(0, enabled);
  if (enabled) 
  {
    Serial.println("Good evening!");
    time_t utc = now();
    time_t local = myTZ.toLocal(utc, &tcr);
    printTime(local, tcr -> abbrev);
    sunrise.StartSunset();
  }
}

void MoonAlarm() {
  bool enabled;
  EEPROM.get(0, enabled);
  if (enabled) 
  {
    Serial.println("Watch out for wherewolves!");
    time_t utc = now();
    time_t local = myTZ.toLocal(utc, &tcr);
    printTime(local, tcr -> abbrev);
    sunrise.StartMoonrise();
  }
}

void MoonSetAlarm() {
  bool enabled;
  EEPROM.get(0, enabled);
  if (enabled) 
  {
    Serial.println("Whew!");
    time_t utc = now();
    time_t local = myTZ.toLocal(utc, &tcr);
    printTime(local, tcr -> abbrev);
    sunrise.StartMoonset();
  }
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

  // Unused
  t.Day = 18;
  t.Month = 11;
  t.Year = 2018 - 1970;
  
  time_t utc = makeTime(t);
  return Alarm.alarmRepeat(hour(utc), m, 0, onTickHandler);
}

int createAlarm(int h, int m, OnTick_t onTickHandler) {
  TimeElements t;
  t.Second = 0;
  t.Minute = m;
  t.Hour = h;
  
  // Unused
  t.Day = 18;
  t.Month = 11;
  t.Year = 2018 - 1970;
  
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
    createAlarm(21, 0, MoonAlarm);
    createAlarm(22, 30, MoonSetAlarm);
    alarmsSet = true;
  }
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

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  int n = WiFi.scanNetworks();
  int found = -1;
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int stored = 0; stored < wifiCount && found == -1; stored++) {
      for (int i = 0; i < n; ++i)
      {
        if (WiFi.SSID(i) == ssids[stored])
        {
          found = stored;
          Serial.print("Connecting to ");
          Serial.println(ssids[found]);
          break;
        }
      }
    }
  }

  if (found > -1)
  {
    //Serial.print("Connecting to ");
    //Serial.println(ssids[found]);

    WiFi.begin(ssids[found], passs[found]);

    bool first = true;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if (first)
        sunrise.SetPixel(0, 50, 0, 0);
      else
        sunrise.SetPixel(0, 0, 0, 50);
    }

    sunrise.SetPixel(0, 0, 50, 0);

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
    sunrise.SetPixel(0, 0, 0, 0);
  } else {
    sunrise.SetPixel(0, 255, 0, 0);
  }

  EEPROM.begin(10);

  server.on("/", handleRoot);
  server.on("/login", handleLogin);
  server.on("/inline", [](){
    server.send(200, "text/plain", "this works without need of authentification");
  });

  server.onNotFound(handleNotFound);
  //here the list of headers to be recorded
  const char * headerkeys[] = {"User-Agent","Cookie"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  //ask server to track these headers
  server.collectHeaders(headerkeys, headerkeyssize );
  server.begin();
  Serial.println("HTTP server started");

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
  // Take care of rollover
  if (milliseconds >= lastTime + 30000 || milliseconds < lastTime) {
    utc = now();
    local = myTZ.toLocal(utc, &tcr);
    printTime(local, tcr -> abbrev);
    lastTime = milliseconds;
    setupAlarms();
  }

  server.handleClient();
}


