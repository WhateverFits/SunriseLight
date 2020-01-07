#include <ArduinoJson.h>
#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <Timezone.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <TM1637.h>
#include <EasyButton.h>
#include <PubSubClient.h>
#include "Config.h"
#include "Sunrise.h"
#include "NTP.h"
#include "Formatting.h"
#include "Webserver.h"

TM1637 tm1637(CLOCK_DIO, CLOCK_CLK);
EasyButton button(BUTTON_PIN);
TimeChangeRule myDST = {"PDT", Second, Sun, Mar, 2, -420};    //Daylight time = UTC - 7 hours
TimeChangeRule mySTD = {"PST", First, Sun, Nov, 2, -480};     //Standard time = UTC - 8 hours
Timezone myTZ(myDST, mySTD);
void mqttPublish(const char* status);
void mqttCallback(char* topic, byte* payload, unsigned int length);

void setupAlarms();
Sunrise sunrise = Sunrise(LEDDELAY, FASTDELAY, LEDS, NEO_PIN, mqttPublish);
Webserver server = Webserver(80, &sunrise, setupAlarms);

long lastTime = 0;
long lastTimeClock = 0;
time_t utc, local;

int morningIndex = -1;
int eveningIndex = -1;
int moonIndex = -1;
int moonSetIndex = -1;

DynamicJsonDocument mqttDoc(1024);
WiFiClient mqttWiFiClient;
String mqttClientId; 
long lastReconnectAttempt = 0; 

PubSubClient mqttClient(MQTT_SERVER, MQTT_PORT, mqttCallback, mqttWiFiClient);

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev
void MorningAlarm() {
  bool enabled;
  EEPROM.get(ENABLEDINDEX, enabled);
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
  EEPROM.get(ENABLEDINDEX, enabled);
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
  bool moonenabled;
  EEPROM.get(ENABLEDINDEX, enabled);
  EEPROM.get(MOONENABLEDINDEX, moonenabled);
  if (enabled && moonenabled) 
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
  bool moonenabled;
  EEPROM.get(ENABLEDINDEX, enabled);
  EEPROM.get(MOONENABLEDINDEX, moonenabled);
  if (enabled && moonenabled) 
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
  return Alarm.alarmOnce(hour(utc), m, 0, onTickHandler);
}

int createAlarm(int h, int m, OnTick_t onTickHandler) {
  TimeElements t;
  t.Second = 0;
  t.Minute = m;
  t.Hour = h;
  time_t holding = now();
  t.Day = day(holding);
  t.Month = month(holding);
  t.Year = year(holding) - 1970;
  
  time_t localTime = makeTime(t);
  time_t utc = myTZ.toUTC(localTime);
  Serial.print("createAlarm: ");
  Serial.print(hour(utc));
  Serial.print(":");
  Serial.println(m);
  return Alarm.alarmOnce(hour(utc), m, 0, onTickHandler);
}

void setupAlarms() {
  for (int i = 0; i < dtNBR_ALARMS; i++) {
    Alarm.free(i);
  }

  getSunriseSunsetTimes();
  moonIndex = createAlarm(21, 0, MoonAlarm);
  moonSetIndex = createAlarm(22, 30, MoonSetAlarm);
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
      Alarm.free(morningIndex);
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
    Alarm.free(morningIndex);
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

void onPressed() {
    String state = (String)sunrise.GetState();
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

void onPressedForDuration() {
    Serial.println("Kill it");
    sunrise.FastToggle();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = (char*)payload;
  Serial.println(message.substring(0, length));
  StaticJsonDocument<1000> doc;
  DeserializationError err= deserializeJson(doc, message.substring(0, length));
  String action = doc["Action"];
  if (action == "Sunrise") sunrise.StartSunrise();
  if (action == "Sunset") sunrise.StartSunset();
  if (action == "Moonrise") sunrise.StartMoonrise();
  if (action == "Moonset") sunrise.StartMoonset();
  if (action == "On") sunrise.On();
  if (action == "Off") sunrise.Off();
  if (action == "FastToggle") sunrise.FastToggle();
}

void mqttPublish(const char* status) {
	if (mqttClient.connected()) {
		mqttDoc["Status"] = status;
		time_t utc = now();
		time_t local = myTZ.toLocal(utc, &tcr);
		mqttDoc["Date"] = formatTime(local, tcr -> abbrev);
		mqttDoc["Percent"] = sunrise.GetPercent();
		mqttDoc["Value"] = sunrise.GetValue();
		char buffer[512];
		size_t n = serializeJson(mqttDoc, buffer);
		mqttClient.publish(MQTT_CHANNEL_PUB, buffer, true);
	}
}

boolean mqttReconnect() {
    char buf[100];
    mqttClientId.toCharArray(buf, 100);
	if (mqttClient.connect(buf, MQTT_USER, MQTT_PASSWORD)) {
		mqttClient.subscribe(MQTT_CHANNEL_SUB);
	}

	return mqttClient.connected();
}

String generateMqttClientId() {
  char buffer[4];
  uint8_t macAddr[6];
  WiFi.macAddress(macAddr);
  sprintf(buffer, "%02x%02x", macAddr[4], macAddr[5]);
  return "SunriseLight" + String(buffer);
}

int findWiFi() {
  // Connect to a WiFi network
  Serial.println("Scanning networks");
  int n = WiFi.scanNetworks();
  int found = -1;
  if (n == 0) {
    Serial.println("no networks found");
    tm1637.dispNumber(9999);
  }
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
          sunrise.SetPixel(0, 50, 0, 0);
          break;
        }
      }
    }
  }

  return found;
}

void connectWiFi(int found) {
  //Serial.print("Connecting to ");
  //Serial.println(ssids[found]);
  sunrise.SetPixel(1, 50, 0, 0);
  #ifdef DNSNAME
  WiFi.hostname(DNSNAME);
  #else
  char buffer[4];
  uint8_t macAddr[6];
  WiFi.macAddress(macAddr);
  sprintf(buffer, "%02x%02x", macAddr[4], macAddr[5]);
  WiFi.hostname("SunriseLight" + String(buffer));
  #endif
  WiFi.begin(ssids[found], passs[found]);
  sunrise.SetPixel(2, 50, 0, 0);

  bool first = true;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (first)
      sunrise.SetPixel(3, 50, 0, 0);
    else
      sunrise.SetPixel(3, 0, 0, 50);
    first != first;
    if (!rtc.lostPower()) {
        utc = rtc.now().unixtime();
        Serial.print("rtc: ");
        Serial.println(utc);
        local = myTZ.toLocal(utc, &tcr);
        tm1637.dispNumber(hour(local) * 100 + minute(local));
    }
  }

  sunrise.SetPixel(4, 50, 0, 0);

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Netmask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());

  sunrise.SetPixel(5, 50, 0, 0);

  Udp.begin(LOCALUDPPORT);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  sunrise.SetPixel(6, 50, 0, 0);
  for (int i = 0; i < 7; i++) {
    sunrise.SetPixel(i, 0, 0, 0);
  }
}

void setupWiFi(){
  int found = -1;
  bool first = true;
  while (found == -1) {
    if (first)
      sunrise.SetPixel(0, 50, 50, 0);
    else
      sunrise.SetPixel(0, 0, 50, 50);
    first != first;
    found = findWiFi();
    if (found == -1) {
      delay(5000);
    }
  }

  connectWiFi(found);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println();

  // Connect to the clock
  tm1637.init();
  tm1637.setBrightness(CLOCKBRIGHT);
  tm1637.dispNumber(0);

  rtc.begin();

  if (!rtc.lostPower()) {
      utc = rtc.now().unixtime();
      Serial.print("rtc: ");
      Serial.println(utc);
      local = myTZ.toLocal(utc, &tcr);
      tm1637.dispNumber(hour(local) * 100 + minute(local));
  } else {
      Serial.print("rtc: lost power.");
  }

  setupWiFi();

  EEPROM.begin(10);
  server.Initialize();

  button.begin();
  // Add the callback function to be called when the button is pressed.
  button.onPressed(onPressed);
  button.onPressedFor(1000, onPressedForDuration);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  mqttClientId = generateMqttClientId();
}

void loop() {
  Alarm.delay(0);
  sunrise.Update();
  button.read();
    
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
	digitalWrite(LED_PIN, LOW);
  }

  server.HandleClient();

  if (!mqttClient.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (mqttReconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    mqttClient.loop();
  }
}
