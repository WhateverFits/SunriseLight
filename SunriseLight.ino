#include <ArduinoJson.h>
#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <Timezone.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <TM1637.h>
#include "Config.h"
#include "Sunrise.h"
#include "NTP.h"
#include "Formatting.h"
#include "Webserver.h"

TM1637 tm1637(CLOCK_DIO, CLOCK_CLK);

TimeChangeRule myDST = {"PDT", Second, Sun, Mar, 2, -420};    //Daylight time = UTC - 7 hours
TimeChangeRule mySTD = {"PST", First, Sun, Nov, 2, -480};     //Standard time = UTC - 8 hours
Timezone myTZ(myDST, mySTD);

void getSunriseSunsetTimes();
Sunrise sunrise = Sunrise(LEDDELAY, LEDS, NEO_PIN);
Webserver server = Webserver(80, &sunrise, getSunriseSunsetTimes);
unsigned long lastButtonTime[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned long lastButtonState[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned long buttonState[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

long lastTime = 0;
long lastTimeClock = 0;
time_t utc, local;

bool alarmsSet = false;
int morningIndex = -1;
int eveningIndex = -1;

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
    alarmsSet = false;
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
  time_t holding = now();
  // Unused
  t.Day = day(holding);
  t.Month = month(holding);
  t.Year = year(holding) - 1970;
  
  time_t localTime = makeTime(t);
  time_t utc = myTZ.toUTC(localTime);
  Serial.print("createAlarm: ");
  Serial.print(hour(utc));
  Serial.print(":");
  Serial.println(m);
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
      StaticJsonDocument<1000> doc;
      DeserializationError err= deserializeJson(doc, line.substring(line.indexOf(':') + 1, line.lastIndexOf(',')));
      //JsonObject& root = jsonBuffer.parseObject(line.substring(line.indexOf(':') + 1, line.lastIndexOf(',')));
      String sunrise = doc["sunrise"];
      String sunset = doc["sunset"];

      Serial.print("Sunrise response: ");
      Serial.println(sunrise);

      String sunriseHour = sunrise.substring(sunrise.indexOf('T') + 1, sunrise.indexOf(':'));
      String sunriseMin = sunrise.substring(sunrise.indexOf(':') + 1, sunrise.indexOf(':') + 3);
      String sunsetHour = sunset.substring(sunset.indexOf('T') + 1, sunset.indexOf(':'));
      String sunsetMin = sunset.substring(sunset.indexOf(':') + 1, sunset.indexOf(':') + 3);
      Serial.print("Sunrise UTC: ");
      Serial.println(sunriseHour + ":" + sunriseMin);
      if (morningIndex > -1)
        Alarm.free(morningIndex);
      if (eveningIndex > -1)
        Alarm.free(eveningIndex);
      bool useSunrise;
      byte hour;
      byte minute;
      EEPROM.get(USESUNRISEINDEX, useSunrise);
      EEPROM.get(FIXEDTIMEINDEX, hour);
      EEPROM.get(FIXEDTIMEINDEX + sizeof(byte), minute);

      if (useSunrise)
      {
        morningIndex = createAlarmUTC(sunriseHour.toInt(), sunriseMin.toInt(), MorningAlarm);
        Serial.println("Sunrise alarm:" + sunriseHour + ":" + sunriseMin);
      } 
      else
      {
        int today = weekday(local);
        if (today != 1 && today != 7)
        {
          morningIndex = createAlarm(hour, minute, MorningAlarm);
          Serial.println("Sunrise alarm:" + String(hour) + ":" + String(minute));
        }
        else
        {
          Serial.print("Dude, sleep in! Today is ");
          Serial.println(dayShortStr(today));
        }
      }
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
  Serial.println();
  Serial.println();

  // Connect to the clock
  tm1637.init();
  tm1637.setBrightness(CLOCKBRIGHT);
  tm1637.dispNumber(0);

  // Set up button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Connect to a WiFi network
  Serial.println("Scanning networks");
  int n = WiFi.scanNetworks();
  int found = -1;
  tm1637.dispNumber(1);
  if (n == 0) {
    Serial.println("no networks found");
    tm1637.dispNumber(9999);
  }
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    tm1637.dispNumber(n);
    for (int stored = 0; stored < wifiCount && found == -1; stored++) {
      for (int i = 0; i < n; ++i)
      {
        if (WiFi.SSID(i) == ssids[stored])
        {
          found = stored;
          Serial.print("Connecting to ");
          Serial.println(ssids[found]);
          tm1637.dispNumber(1111);
          break;
        }
      }
    }
  }

  if (found > -1)
  {
    //Serial.print("Connecting to ");
    //Serial.println(ssids[found]);
    tm1637.dispNumber(2222);
    uint8_t macAddr[6];
    WiFi.macAddress(macAddr);
    char buffer[4];
    sprintf(buffer, "%02x%02x", macAddr[4], macAddr[5]);
    WiFi.hostname("SunriseLight" + String(buffer));
    WiFi.begin(ssids[found], passs[found]);
    tm1637.dispNumber(3333);
    
    bool first = true;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if (first)
        sunrise.SetPixel(0, 50, 0, 0);
      else
        sunrise.SetPixel(0, 0, 0, 50);
    }

    tm1637.dispNumber(4444);
    sunrise.SetPixel(0, 0, 50, 0);

    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Netmask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());

    tm1637.dispNumber(5555);
    
    Udp.begin(LOCALUDPPORT);
    Serial.print("Local port: ");
    Serial.println(Udp.localPort());
    Serial.println("waiting for sync");
    setSyncProvider(getNtpTime);
    sunrise.SetPixel(0, 0, 0, 0);
    tm1637.dispNumber(6666);
  } else {
    sunrise.SetPixel(0, 255, 0, 0);
  }

  EEPROM.begin(10);
  server.Initialize();
  tm1637.dispNumber(7777);
}

void loop() {
  Alarm.delay(0);
  sunrise.Update();

  if (debounceButton(BUTTON_PIN)) {
    
    String state = sunrise.GetState();
    if (state == "Off") {
      Serial.println("Sunrise");
      sunrise.StartSunrise();
    } else if (state == "Sunrise") {
      Serial.println("Sunset");
      sunrise.StartSunset();
    } else if (state == "Sunset") {
      Serial.println("Moonrise");
      sunrise.StartMoonrise();
    } else {
      Serial.println("Moonset");
      sunrise.StartMoonset();
    }
  }

  long milliseconds = millis();
  utc = now();
  local = myTZ.toLocal(utc, &tcr);

  // Check the time. Set alarms. Take care of rollover
  if (milliseconds >= lastTime + 7200000 || milliseconds < lastTime || lastTime == 0) {
    printTime(local, tcr -> abbrev);
    lastTime = milliseconds;
    setupAlarms();
  }

  // Update clock display. Take care of rollover
  if (milliseconds >= lastTimeClock + 1000 || milliseconds < lastTimeClock) {
    //Serial.println("Sending time to LED 7 Segment");
    lastTimeClock = milliseconds;
    tm1637.dispNumber(hour(local) * 100 + minute(local));
    tm1637.switchColon();
  }

  server.HandleClient();
}
