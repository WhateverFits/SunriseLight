#include <ArduinoJson.h>
#include <TimeLib.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <Timezone.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <TM1637.h>
#include <EasyButton.h>
#include <PubSubClient.h>
#include <stdint.h>
#include <RTClib.h>
#include <jled.h>
#include "SunriseLight.h"
#include "Config.h"
#include "Sunrise.h"
#include "Formatting.h"
#include "Webserver.h"

// MQTT method headers
void mqttPublish(const char* status);
void mqttCallback(char* topic, byte* payload, unsigned int length);

// Alarm header
void setupAlarms();

// Display
TM1637 tm1637(CLOCK_DIO, CLOCK_CLK);

// Sunrise
Sunrise sunrise = Sunrise(LEDDELAY, FASTDELAY, LEDS, NEO_PIN, mqttPublish);

// Web Server
Webserver server = Webserver(80, &sunrise, setupAlarms);

// WiFi connection
ESP8266WiFiMulti wifiMulti;
WiFiClient mqttWiFiClient;
PubSubClient mqttClient(MQTT_SERVER, MQTT_PORT, mqttCallback, mqttWiFiClient);

// NTP and RTC
RTC_DS3231 rtc;
WiFiUDP Udp;

// Control button
EasyButton button(BUTTON_PIN);
EasyButton button2(BUTTON2_PIN);
// LED on control button
auto led = JLed(LED_PIN);
auto led2 = JLed(LED2_PIN);

// TimeZone rules
TimeChangeRule myDST = {"PDT", Second, Sun, Mar, 2, -420};    //Daylight time = UTC - 7 hours
TimeChangeRule mySTD = {"PST", First, Sun, Nov, 2, -480};     //Standard time = UTC - 8 hours
Timezone myTZ(myDST, mySTD);

// Locals
long lastTime = 0;
long lastTimeClock = 0;
time_t utc, local;

// Alarms
int morningIndex = -1;
int eveningIndex = -1;
int moonIndex = -1;
int moonSetIndex = -1;

bool connectedOnce = false;

String mqttClientId; 
long lastReconnectAttempt = 0; 

int ntpRetryCount = 3;
int ntpRetry = 0;
long rtcCount = RTCSTALECOUNT;
int displayBrightness = CLOCKBRIGHT;

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
  t.Year = year(now()) - 1970;

  time_t utc = makeTime(t);
  mqttLog(("UTC Alarm set for " + String(h) + ":" + String(m)));
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
  mqttLog("setupAlarms - Enter");
  for (int i = 0; i < dtNBR_ALARMS; i++) {
    Alarm.free(i);
  }

  getSunriseSunsetTimes();
  mqttLog("setupAlarms - Exit");
}

void getSunriseSunsetTimes() {
  mqttLog("getSunriseSunsetTimes - Enter");
  WiFiClientSecure client;
  const int httpPort = 443;
  const char* host = "api.sunrise-sunset.org";
  client.setInsecure();
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    mqttLog("connection failed");
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
      mqttLog("Received sunrise");
      StaticJsonDocument<1000> doc;
      DeserializationError err = deserializeJson(doc, line.substring(line.indexOf(':') + 1, line.lastIndexOf(',')));
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

      int sunsetMinInt = sunsetMin.toInt();
      int sunsetHourInt = sunsetHour.toInt();
      Serial.print("Sunset UTC: ");
      Serial.println(String(sunsetHourInt) + ":" + String(sunsetMinInt));
      eveningIndex = createAlarmUTC(sunsetHourInt, sunsetMinInt, EveningAlarm);

      // Make moonrise
      sunsetMinInt += 20;
      if (sunsetMinInt > 60) {
        sunsetMinInt -= 60;
        sunsetHourInt++;
        if (sunsetHourInt == 24) {
          sunsetHourInt = 0;
        }
      }

      Serial.print("Moonrise UTC: ");
      Serial.println(String(sunsetHourInt) + ":" + String(sunsetMinInt));
      moonIndex = createAlarmUTC(sunsetHourInt, sunsetMinInt, MoonAlarm);

      // Make moonset
      sunsetMinInt += 30;
      if (sunsetMinInt > 60) {
        sunsetMinInt -= 60;
        sunsetHourInt++;
        if (sunsetHourInt == 24) {
          sunsetHourInt = 0;
        }
      }

      Serial.print("Moonset UTC: ");
      Serial.println(String(sunsetHourInt) + ":" + String(sunsetMinInt));
      moonSetIndex = createAlarmUTC(sunsetHourInt, sunsetMinInt, MoonSetAlarm);
      break;
    }
  }

  Serial.println();
  Serial.println("closing connection");
  mqttLog("getSunriseSunsetTimes - Exit");
}

// When the button is short pressed, execute this
void onPressed() {
  bool sunrising = sunrise.Toggle();
  led.Off();
  led.Blink(200, 800).Repeat(1);
  if (sunrising) {
    tm1637.display("RISE");
  } else {
    tm1637.display("SET ");
  }

  // Reset the clock timer so that the RISE or SET displays for a second
  lastTimeClock = millis();
}

// When the button is held for 1000 ms, execute this
void onPressedForDuration() {
  Serial.println("Fast Toggle from long press");
  bool sunrising = sunrise.FastToggle();
  led.Off();
  led.Blink(200, 200).Repeat(5);
  if (sunrising) {
    tm1637.display("RISE");
  } else {
    tm1637.display("SET ");
  }

  // Reset the clock timer so that the RISE or SET displays for a second
  lastTimeClock = millis();
}

// When the button is short pressed, execute this
void onPressed2() {
  led2.Off();
  led2.Blink(200, 800).Repeat(1);
  mqttClient.publish(MQTT_CHANNEL_SCENE, "On", true);

  // Reset the clock timer so that the RISE or SET displays for a second
  lastTimeClock = millis();
}

void displayBright() {
  if (++displayBrightness > 7) displayBrightness = 7;
  mqttLog("displayBright: " + String(displayBrightness));
  tm1637.setBrightness(displayBrightness);
}

void displayDim() {
  if (--displayBrightness < 1) displayBrightness = 1;
  mqttLog("displayDim: " + String(displayBrightness));
  tm1637.setBrightness(displayBrightness);
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
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
  //Udp.beginMulticast(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
    String(ipAddress[1]) + String(".") +\
    String(ipAddress[2]) + String(".") +\
    String(ipAddress[3])  ;
}

time_t getNtpTime()
{
  // Check if we have a time and it isn't too stale.
  if (!rtc.lostPower() && rtcCount < RTCSTALECOUNT) {
    rtcCount++;
    Serial.println("Returning RTC value");
    long u = rtc.now().unixtime();
    Serial.print("rtc: ");
    Serial.println(u);
    return u;
  }


  if (wifiMulti.run() == WL_CONNECTED) {
    while (Udp.parsePacket() > 0) ; // discard any previously received packets
    // Fall back to using the global NTP pool server in case we cannot connect to internal NTP server
    if (ntpRetry > 1)
      WiFi.hostByName(ntpServerName, timeServer);
    Serial.print("Transmit NTP Request to ");
    Serial.println(IpAddress2String(timeServer));
    sendNTPpacket(timeServer);
    uint32_t beginWait = millis();
    byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
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
        randomSeed(secsSince1900);
        // Make sure we chill a little
        setSyncInterval(200);
        unsigned long calcTime = secsSince1900 - 2208988800UL;// + timeZone * SECS_PER_HOUR;
        rtc.adjust(DateTime(calcTime));
        DateTime timeNow = rtc.now();
        long u = timeNow.unixtime();
        Serial.print("rtc: ");
        Serial.println(u);
        Serial.print("ntp: ");
        Serial.println(calcTime);
        // Reset the RTC stale counter. 
        rtcCount = 0;
        return calcTime;
      }
    }
  }
  Serial.println("No NTP Response :-(");
  if (ntpRetry < ntpRetryCount) 
  {
    ntpRetry++;
    return getNtpTime();
  }

  // We couldn't connect so we are gonna try harder!
  Serial.println("NTP n");
  setSyncInterval(5);
  if (!rtc.lostPower()) {
    Serial.println("NTP o");
    return rtc.now().unixtime(); // return now if unable to get the time so we just get our RTC's time.
  } else {
    Serial.println("NTP p");
    return now(); // return now if unable to get the time so we just get our drifted internal time instead of wrong time.
  }
}

// The callback when an MQTT message is received. We only listen to one topic (control)
// so we only pay attenttion to the message/payload/action.
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Inside mqtt callback: %s\n", topic);
  Serial.println(length);

  // This payload can be the entire buffer. Make sure to only use the provided length of it.
  String action = (char*)payload;
  action = action.substring(0, length);
  Serial.println(action);

  if (action == "Sunrise") sunrise.StartSunrise();
  if (action == "Sunset") sunrise.StartSunset();
  if (action == "Moonrise") sunrise.StartMoonrise();
  if (action == "Moonset") sunrise.StartMoonset();
  if (action == "On") sunrise.On();
  if (action == "Off") sunrise.Off();
  if (action == "DisplayBright") displayBright();
  if (action == "DisplayDim") displayDim();
  if (action == "FastToggle") {
    bool sunrising = sunrise.FastToggle();
    if (sunrising) {
      tm1637.display("RISE");
    } else {
      tm1637.display("SET ");
    }
    // Reset the clock timer so that the RISE or SET displays for a second
    lastTimeClock = millis();
  }
  if (action == "Update") {
    WiFiClient updateWiFiClient;
    t_httpUpdate_return ret = ESPhttpUpdate.update(updateWiFiClient, UPDATE_URL);
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
  }
}

// Send out the provided message in JSON format to home/DNSNAME/state
void mqttPublish(const char* status) {
  if (mqttClient.connected()) {
    DynamicJsonDocument mqttDoc(1024);
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

void mqttLog(const char* status) {
  Serial.println(status);
  if (mqttClient.connected()) {
    mqttClient.publish(MQTT_CHANNEL_LOG, status, true);
  }
}

void mqttLog(String status) {
  char buf[256];
  status.toCharArray(buf, 256);
  mqttLog(buf);
}

boolean mqttReconnect() {
  char buf[100];
  mqttClientId.toCharArray(buf, 100);
  if (mqttClient.connect(buf, MQTT_USER, MQTT_PASSWORD)) {
    mqttClient.subscribe(MQTT_CHANNEL_SUB);
    Serial.println("MQTT connected");
  }

  return mqttClient.connected();
}

String generateMqttClientId() {
  char buffer[4];
  uint8_t macAddr[6];
  WiFi.macAddress(macAddr);
  sprintf(buffer, "%02x%02x", macAddr[4], macAddr[5]);
#ifdef DNSNAME
  return DNSNAME + String(buffer);
#else
  return "SunriseLight" + String(buffer);
#endif
}

void updateStarted() {
  Serial.println("CALLBACK:  HTTP update process started");
  sunrise.Off();
}

void updateFinished() {
  Serial.println("CALLBACK:  HTTP update process finished");
  sunrise.SetValue(0, 50, 0);
}

void updateProgress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
  tm1637.display(cur * 100 / total);

  for (int i = 0; i < (LEDS * cur / total); i++) 
    sunrise.SetPixel(i, 0, 0, 50);
  sunrise.StripShow();
}

void updateError(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);

  switch(event) {
    case WIFI_EVENT_STAMODE_GOT_IP:
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      onConnect();
      break;
    case WIFI_EVENT_STAMODE_DISCONNECTED:
      Serial.println("WiFi lost connection");
      WiFi.disconnect();
      connectedOnce = false;
      delay(500);
      wifiMulti.run();
      break;
  }
}

void onConnect() {
  Serial.print("Connected: ");
  Serial.println(WiFi.localIP());
  connectedOnce = true;
  Udp.begin(LOCALUDPPORT);
  setSyncProvider(getNtpTime);
}

void setupWiFi(){
  WiFi.disconnect();
  delay(1000);
  //WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_STA);
#ifdef DNSNAME
  WiFi.hostname(DNSNAME);
#else
  char buffer[4];
  uint8_t macAddr[6];
  WiFi.macAddress(macAddr);
  sprintf(buffer, "%02x%02x", macAddr[4], macAddr[5]);
  WiFi.hostname("SunriseLight" + String(buffer));
#endif

  for (int i=0; i < wifiCount; i++) {
    wifiMulti.addAP(ssids[i], passs[i]);
  }
  Serial.println("Connecting");
  wifiMulti.run();
  //WiFi.begin(ssids[0], passs[0]);
}

bool validateWiFi(long milliseconds) {
  //return connectedOnce;
  // Update WiFi status. Take care of rollover
  if (milliseconds >= lastTimeClock + 1000 || milliseconds < lastTimeClock) {
    if (wifiMulti.run() != WL_CONNECTED) {
      Serial.println("Disconnected");
      connectedOnce = false;
    } else {
      if (!connectedOnce) {
        Serial.print("Connected late to ");
        Serial.println(WiFi.SSID());
        onConnect();
      }

      connectedOnce = true;
    }
  }

  return connectedOnce;
}

void validateMqtt(long milliseconds) {
  if (!mqttClient.connected()) {
    if (milliseconds - lastReconnectAttempt > 5000 || lastReconnectAttempt == 0 || milliseconds < lastReconnectAttempt) {
      Serial.println("MQTT not connected");
      lastReconnectAttempt = milliseconds;
      Serial.println("MQTT reconnecting");
      // Attempt to reconnect
      if (mqttReconnect()) {
        Serial.println("MQTT reconnected");
      }
    }

    if (milliseconds - lastReconnectAttempt > 60000) {
      Serial.println("MQTT disconnecting WiFi");
      WiFi.disconnect();
      delay(500);
    }
  } else {
    mqttClient.loop();
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println();

  // Connect to the clock
  tm1637.init();
  tm1637.setBrightness(CLOCKBRIGHT);
  tm1637.display(0);

  rtc.begin();

  if (!rtc.lostPower()) {
    utc = rtc.now().unixtime();
    Serial.print("rtc: ");
    Serial.println(utc);
    local = myTZ.toLocal(utc, &tcr);
    tm1637.display(hour(local) * 100 + minute(local));
  } else {
    Serial.println("rtc: lost power.");
  }

  setupWiFi();

  EEPROM.begin(10);
  server.Initialize();

  button.begin();
  // Add the callback function to be called when the button is pressed.
  button.onPressed(onPressed);
  button.onPressedFor(1000, onPressedForDuration);
  led.Off();

  button2.onPressed(onPressed2);
  led2.Off();

  mqttClientId = generateMqttClientId();

  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
  ESPhttpUpdate.onStart(updateStarted);
  ESPhttpUpdate.onEnd(updateFinished);
  ESPhttpUpdate.onProgress(updateProgress);
  ESPhttpUpdate.onError(updateError);
}

void loop() {
  //wifiMulti.run();

  Alarm.delay(0);
  button.read();
  button2.read();
  sunrise.Update();

  if (button.isPressed() && !led.IsRunning()) {
    led.On();
  }   

  if (button2.isPressed() && !led2.IsRunning()) {
    led2.On();
  }   

  led.Update();
  led2.Update();

  long milliseconds = millis();

  // Check if connected then handle the connected magic
  if (validateWiFi(milliseconds)) {
    validateMqtt(milliseconds);
    server.HandleClient();

    // Check the time. Set alarms. Take care of rollover
    if (milliseconds >= lastTime + 7200000 || milliseconds < lastTime || lastTime == 0 ) {
      printTime(local, tcr -> abbrev);
      lastTime = milliseconds;
      setupAlarms();
    }

    utc = now();
  } else {
    utc = rtc.now().unixtime();
  }

  // Convert to local time what was gathered above
  local = myTZ.toLocal(utc, &tcr);

  // Update clock display. Take care of rollover
  if (milliseconds >= lastTimeClock + 1000 || milliseconds < lastTimeClock) {
    //Serial.println("Sending time to LED 7 Segment");
    lastTimeClock = milliseconds;
    tm1637.display(hour(local) * 100 + minute(local));
    tm1637.switchColon();
    lastTimeClock = milliseconds;
  }
}
