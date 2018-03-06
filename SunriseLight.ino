#include <ArduinoJson.h>

#include <TimeAlarms.h>

#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <Timezone.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include "Config.h"
#include "Sunrise.h"
#include "NTP.h"

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

long lastTime = 0;
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
        morningIndex = createAlarm(hour, minute, MorningAlarm);
        Serial.println("Sunrise alarm:" + String(hour) + ":" + String(minute));
      }
      eveningIndex = createAlarmUTC(sunsetHour.toInt(), sunsetMin.toInt(), EveningAlarm);
      break;
    }
  }

  Serial.println();
  Serial.println("closing connection");
}

ESP8266WebServer server(80);

//Check if header is present and correct
bool is_authentified(){
  unsigned int authkey=-1;
  EEPROM.get(AUTHKEYINDEX, authkey);
  Serial.println("Enter is_authentified");
  if (server.hasHeader("Cookie")){   
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
    Serial.print("Found authkey: ");
    Serial.println(authkey);
    if (cookie.indexOf("ESPSESSIONID=" + String(authkey)) != -1) {
      Serial.println("Authentification Successful");
      return true;
    }
  }
  Serial.println("Authentification Failed");
  return false;  
}

String genCSS()
{
  return String("<meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1, user-scalable=0'><link rel='stylesheet' href='https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/1.12.4/jquery.min.js'></script><script src='https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.js'></script><style>.pb{border:1px solid black;}.ui-title{margin: 0 0 !important;}</style>");
  //return String("<meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1, user-scalable=0'><style>form{width:80%;margin:0 auto;}label,input{display:inline-block;}label{width:40%;text-align:right;}label+input{width:30%;margin:0 20% 2% 4%;}.l{display:block;text-align:left;margin:0 auto 2% auto;width:60%;}.h{width:10%;margin-right:0px}</style>");
}


//login page, also called for disconnect
void handleLogin(){
  String msg;
  if (server.hasHeader("Cookie")){   
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
  }
  if (server.hasArg("DISCONNECT")){
    Serial.println("Disconnection");
    String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=0\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    return;
  }
  if (server.hasArg("USERNAME") && server.hasArg("PASSWORD")){
    if (server.arg("USERNAME") == "admin" &&  server.arg("PASSWORD") == "admin" ){
      unsigned int authkey = random(65535);
      EEPROM.put(AUTHKEYINDEX, authkey);
      EEPROM.commit();
      String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=" + String(authkey) + "\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
      server.sendContent(header);
      Serial.println("Log in Successful");
      return;
    }
    msg = "Wrong username/password! try again.";
    Serial.println("Log in Failed");
  }
  String content = "<html>" + genCSS() + "<body data-role='page'><div data-role='header'><H2>Sunrise Alarm Control</H2></div><div class='ui-content'><form action='/login' method='POST' data-ajax='false'>";
  content += "<label>User:</label><input type='text' name='USERNAME' placeholder='user name'>";
  content += "<label>Password:</label><input type='password' name='PASSWORD' placeholder='password'>";
  content += "<label></label><input type='submit' name='SUBMIT' value='Submit'></form>" + msg;
  content += "You also can go <a href='/inline'>here</a></div></body></html>";
  server.send(200, "text/html", content);
}

//root page can be accessed only if authentification is ok
void handleRoot(){
  Serial.println("Enter handleRoot");
  String header;
  if (!is_authentified()){
    String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    return;
  }
  String content = "<html>" + genCSS() + "<body data-role='page'><div data-role='header'><H2>Sunrise Alarm Control</H2></div><div class='ui-content'>";
  String msg = "";
  bool enabled = true;
  if (server.hasArg("ENABLE")){
    enabled = server.arg("ENABLE") == "enabled";
    EEPROM.put(ENABLEDINDEX, enabled);
    EEPROM.commit();
  } else {
    EEPROM.get(ENABLEDINDEX, enabled);
  }

  bool useSunrise = true;
  byte hour = 255;
  byte minute = 255;
  if (server.hasArg("FIXED")){
    useSunrise = server.arg("FIXED") != "fixed";
    EEPROM.put(USESUNRISEINDEX, useSunrise);
    if (!useSunrise)
    {
      Serial.println("TIME: " + server.arg("TIME"));
      int idx = server.arg("TIME").indexOf(":");
      if (idx > 0) 
      {
        Serial.println("inside:" + String(idx));
        hour = server.arg("TIME").substring(0, idx).toInt();
        minute = server.arg("TIME").substring(idx + 1).toInt();
        Serial.print(hour);
        Serial.print(":");
        Serial.println(minute);
        EEPROM.put(FIXEDTIMEINDEX, hour);
        EEPROM.put(FIXEDTIMEINDEX + sizeof(byte), minute);
        
      }
    }
    
    EEPROM.commit();
  } else {
    EEPROM.get(USESUNRISEINDEX, useSunrise);
  }

  if (server.hasArg("SUNRISE") && server.arg("SUNRISE") == "Sunrise"){
    sunrise.StartSunrise();
  }

  if (server.hasArg("SUNSET") && server.arg("SUNSET") == "Sunset"){
    sunrise.StartSunset();
  }

  if (!useSunrise)
  {
      EEPROM.get(FIXEDTIMEINDEX, hour);
      EEPROM.get(FIXEDTIMEINDEX + sizeof(byte), minute);
      if (hour >= 24 || minute >= 60) 
      {
        hour = 255;
        minute = 255;
      }
  }

  char minbuf[4];
  sprintf(minbuf, "%02d", minute);

  char hourbuf[4];
  sprintf(hourbuf, "%02d", hour);

  msg += "<br/>" + sunrise.GetState();

  content += "<form action='/' method='POST' data-ajax='false'>";
  content += "<label class='l'><input type='radio' name='ENABLE' value='enabled' " + (enabled ? String("checked") : String("")) + ">Enabled</label>";
  content += "<label class='l'><input type='radio' name='ENABLE' value='disabled' " + (!enabled ? String("checked") : String("")) + ">Disabled</label>";

  content += "<label class='l'><input type='radio' name='FIXED' value='sunrise' " + (useSunrise ? String("checked") : String("")) + ">Use Sunrise Time</label>";
  content += "<label class='l'><input type='radio' name='FIXED' value='fixed' " + (!useSunrise ? String("checked") : String("")) + ">Use Fixed Time</label>";
  content += "<div id='fixedWrap'><label>Fixed time: </label><input type='time' data-clear-btn='true' name='TIME' id='time' value='" + (hour < 24 ? String(hourbuf) + ":" : String("") ) + (minute < 60 ? String(minbuf) : String("") ) + "'></div>";
  //content += "<label>Fixed time:</label><input type='text' name='HOUR' placeholder='hour' class='h' value='" + (hour < 24 ? String(hour) : String("") ) + "'><input type='text' name='MINUTE' placeholder='min' class='h' value='" + (minute < 60 ? String(buffer) : String("") ) + "'>";

  content += "<div><button name='SUNRISE' value='Sunrise'>Sunrise</button><button name='SUNSET' value='Sunset'>Sunset</button>";
  content += "<button name='SUBMIT' value='Save'>Save</button></div></form></div><div data-role='footer'><h3>" + msg + "</h3>";
  content += "<div class='pb' style='width:" + String(sunrise.GetPercent()) + "%;background-color:#" + sunrise.GetColor() + "'>&nbsp;</div>";
  content += "You can access this page until you <a href=\"/login?DISCONNECT=YES\">disconnect</a></div>" + javascript() + "</body></html>";
  server.send(200, "text/html", content);
}

String javascript() {
  return "<script>$(document).ready(function() {$('input[type=radio][name=FIXED]').change(function() { if ($('input[name=""FIXED""]:checked').val() == 'fixed') { $('#fixedWrap').show(); } else { $('#fixedWrap').hide(); } } ).change();});</script>";
}

//no need authentification
void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
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

    uint8_t macAddr[6];
    WiFi.macAddress(macAddr);
    char buffer[4];
    sprintf(buffer, "%02x%02x", macAddr[4], macAddr[5]);
    WiFi.hostname("SunriseLight" + String(buffer));
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


