#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "Config.h"

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
  return String("<meta name='viewport' content='width=device-width, initial-scale=1'><style>form{width:80%;margin:0 auto;}label,input{display:inline-block;}label{width:40%;text-align:right;}label+input{width:30%;margin:0 30% 2% 4%;}.l{display:block;text-align:left;}.h{width:10%;margin-right:0px}</style>");
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
  String content = "<html>" + genCSS() + "<body><H2>Sunrise Alarm Control</H2><form action='/login' method='POST'>";
  content += "<label>User:</label><input type='text' name='USERNAME' placeholder='user name'>";
  content += "<label>Password:</label><input type='password' name='PASSWORD' placeholder='password'>";
  content += "<label></label><input type='submit' name='SUBMIT' value='Submit'></form>" + msg;
  content += "You also can go <a href='/inline'>here</a></body></html>";
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
  String content = "<html>" + genCSS() + "<body><H2>Sunrise Alarm Control</H2>";
  String msg = "";
  bool enabled = true;
  if (server.hasArg("ENABLE")){
    msg = "The alarm has been " + server.arg("ENABLE");
    enabled = server.arg("ENABLE") == "enabled";
    EEPROM.put(ENABLEDINDEX, enabled);
    EEPROM.commit();
  } else {
    EEPROM.get(ENABLEDINDEX, enabled);
    msg = "The alarm is currently " + String(enabled ? "enabled" : "disabled");
  }

  bool useSunrise = true;
  byte hour = 255;
  byte minute = 255;
  if (server.hasArg("FIXED")){
    msg += " Using time: " + server.arg("FIXED");
    useSunrise = server.arg("FIXED") != "fixed";
    EEPROM.put(USESUNRISEINDEX, useSunrise);
    if (!useSunrise)
    {
      hour = server.arg("HOUR").toInt();
      minute = server.arg("MINUTE").toInt();
      EEPROM.put(FIXEDTIMEINDEX, hour);
      EEPROM.put(FIXEDTIMEINDEX + sizeof(byte), minute);
    }
    
    EEPROM.commit();
  } else {
    EEPROM.get(USESUNRISEINDEX, useSunrise);
    msg += " Using " + String(useSunrise ? "sunrise" : "fixed");
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

  content += "<form action='/' method='POST'>";
  content += "<label class='l'><input type='radio' name='ENABLE' value='enabled' " + (enabled ? String("checked") : String("")) + ">Enabled</label>";
  content += "<label class='l'><input type='radio' name='ENABLE' value='disabled' " + (!enabled ? String("checked") : String("")) + ">Disabled</label>";

  content += "<label class='l'><input type='radio' name='FIXED' value='sunrise' " + (useSunrise ? String("checked") : String("")) + ">Use Sunrise Time</label>";
  content += "<label class='l'><input type='radio' name='FIXED' value='fixed' " + (!useSunrise ? String("checked") : String("")) + ">Use Fixed Time</label>";
  content += "<label>Fixed time:</label><input type='text' name='HOUR' placeholder='hour' class='h' value='" + (hour < 24 ? String(hour) : String("") ) + "'><input type='text' name='MINUTE' placeholder='min' class='h' value='" + (minute < 60 ? String(minute) : String("") ) + "'>";

  content += "<label></label><input type='submit' name='SUBMIT' value='Submit'></form><h3>" + msg + "</h3>";
  content += "You can access this page until you <a href=\"/login?DISCONNECT=YES\">disconnect</a></body></html>";
  server.send(200, "text/html", content);
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

