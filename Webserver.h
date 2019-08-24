#include <ESP8266WebServer.h>
#include <time.h>
#include "Sunrise.h"

class Webserver {
  public: 
    typedef std::function<void(void)> THandlerFunction;
    Webserver(int port, Sunrise *sunrise, THandlerFunction recalculate) {
      server = new ESP8266WebServer(port);
      _sunrise = sunrise;
    }

    void Initialize() {
        server->on("/", [this](){ handleRoot(); });
        server->on("/login", [this](){ handleLogin(); });
        server->on("/status", [this](){ handleStatus(); });
      
        server->on("/debug", [this](){ handleDebug(); });
        server->onNotFound([this](){ handleNotFound(); });
        
        //here the list of headers to be recorded
        const char * headerkeys[] = {"User-Agent","Cookie"} ;
        size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
        //ask server to track these headers
        server->collectHeaders(headerkeys, headerkeyssize );
        server->begin();
        Serial.println("HTTP server started");

    }

    void HandleClient() {
      server->handleClient();
    };

  private:
    int _delay;
    ESP8266WebServer *server = NULL;
    Sunrise *_sunrise = NULL;
    THandlerFunction _recalculate;

    
    //Check if header is present and correct
    bool is_authenticated(){
      unsigned int authkey=-1;
      EEPROM.get(AUTHKEYINDEX, authkey);
      if (server->hasHeader("Cookie")){   
        String cookie = server->header("Cookie");
        //Serial.print("Found authkey: ");
        //Serial.println(authkey);
        if (cookie.indexOf("ESPSESSIONID=" + String(authkey)) != -1) {
          //Serial.println("Authentification Successful");
          return true;
        }
      }
      Serial.println("Authentification Failed");
      return false;  
    }
    
    String genCSS()
    {
      return String("<head>\n\
      <meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1, user-scalable=0'>\n\
      <link rel='stylesheet' href='https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.css'>\n\
      <script src='https://ajax.googleapis.com/ajax/libs/jquery/1.12.4/jquery.min.js'></script>\n\
      <script src='https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.js'></script>\n\
      <style>.pb{border:1px solid black;}.ui-title{margin: 0 0 !important;}</style>\n\
      <script>\n\
      function repeater(){\n\
      $.getJSON('/status', function(data) {\n\
      $('#pb').css('width', data.percent + '%');\n\
      $('#pb').css('background-color', '#' + data.color);\n\
      $('#state').text(data.state);\n\
      setTimeout(repeater, " + String(LEDDELAY) + ");\n\
      }, function(data) {\n\
      $('#state').text('error');\n\
      setTimeout(repeater, " + String(LEDDELAY) + ");\n\
      });\n\
      }\n\
      </script>\n\
      </head>\n");
      //return String("<meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1, user-scalable=0'><style>form{width:80%;margin:0 auto;}label,input{display:inline-block;}label{width:40%;text-align:right;}label+input{width:30%;margin:0 20% 2% 4%;}.l{display:block;text-align:left;margin:0 auto 2% auto;width:60%;}.h{width:10%;margin-right:0px}</style>");
    }
    
    
    //login page, also called for disconnect
    void handleLogin(){
      String msg;
      if (server->hasHeader("Cookie")){   
        Serial.print("Found cookie: ");
        String cookie = server->header("Cookie");
        Serial.println(cookie);
      }
      if (server->hasArg("DISCONNECT")){
        Serial.println("Disconnection");
        String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=0\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
        server->sendContent(header);
        return;
      }
      if (server->hasArg("USERNAME") && server->hasArg("PASSWORD")){
        if (server->arg("USERNAME") == "admin" &&  server->arg("PASSWORD") == "admin" ){
          unsigned int authkey = random(65535);
          EEPROM.put(AUTHKEYINDEX, authkey);
          EEPROM.commit();
          String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=" + String(authkey) + "\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
          server->sendContent(header);
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
      content += "</div></body></html>";
      server->send(200, "text/html", content);
    }

    void handleStatus() {
      if (is_authenticated()){
        server->sendHeader("Cache-Control", "no-cache");
        String content = "{ \"percent\": " + String(_sunrise->GetPercent()) + ", \"color\": \"" + _sunrise->GetColor() + "\", \"state\": \"" + _sunrise->GetState() + "\" }";
        server->send(200, "application/json", content);
      } else {
        server->send(401, "text/plain", "Not authorized.");
      }
    }
    
    void handleDebug() {
      String content = "";
      for (int i = 0; i < dtNBR_ALARMS; i++) {
        char buffer[80];
        
        struct tm * timeinfo;
        time_t daTime = Alarm.read(i);
        timeinfo = localtime (&daTime);
        strftime(buffer, 80, "%I:%M%p", timeinfo);
        content += String(i) + " - " + String(buffer);
      }

      server->send(200, "text/plain", content);
    }
    
    //root page can be accessed only if authentification is ok
    void handleRoot(){
      Serial.println("Enter handleRoot");
      String header;
      if (!is_authenticated()){
        String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
        server->sendContent(header);
        return;
      }
      String content = "<html>" + genCSS() + "<body data-role='page'><div data-role='header'><H2>Sunrise Alarm Control</H2></div><div class='ui-content'>";
      bool enabled = true;
      bool moonenabled = false;
      if (server->hasArg("ENABLE")){
        enabled = server->arg("ENABLE") == "enabled";
        EEPROM.put(ENABLEDINDEX, enabled);
        EEPROM.commit();
      } else {
        EEPROM.get(ENABLEDINDEX, enabled);
      }
    
      if (server->hasArg("MOONENABLE")){
        moonenabled = server->arg("MOONENABLE") == "enabled";
        EEPROM.put(MOONENABLEDINDEX, moonenabled);
        EEPROM.commit();
      } else {
        EEPROM.get(MOONENABLEDINDEX, moonenabled);
      }
    
      bool useSunrise = true;
      byte hour = 255;
      byte minute = 255;
      if (server->hasArg("FIXED")){
        useSunrise = server->arg("FIXED") != "fixed";
        EEPROM.put(USESUNRISEINDEX, useSunrise);
        if (!useSunrise)
        {
          Serial.println("TIME: " + server->arg("TIME"));
          int idx = server->arg("TIME").indexOf(":");
          if (idx > 0) 
          {
            Serial.println("Saving time to EEPROM");
            hour = server->arg("TIME").substring(0, idx).toInt();
            minute = server->arg("TIME").substring(idx + 1).toInt();
            EEPROM.put(FIXEDTIMEINDEX, hour);
            EEPROM.put(FIXEDTIMEINDEX + sizeof(byte), minute);
    
            // getSunriseSunsetTimes();
            if (_recalculate) {
              _recalculate();
            }
          }
        }
        
        EEPROM.commit();
      } else {
        EEPROM.get(USESUNRISEINDEX, useSunrise);
      }
    
      if (server->hasArg("SUNRISE") && server->arg("SUNRISE") == "Sunrise"){
        _sunrise->StartSunrise();
      }
    
      if (server->hasArg("SUNSET") && server->arg("SUNSET") == "Sunset"){
        _sunrise->StartSunset();
      }
    
      if (server->hasArg("OFF") && server->arg("OFF") == "Off"){
        _sunrise->SetValue(0, 0, 0);
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
    
      content += "<form action='/' method='POST' data-ajax='false'>\n";
      content += "<label class='l'><input type='radio' name='ENABLE' value='enabled' " + (enabled ? String("checked") : String("")) + ">Enabled</label>\n";
      content += "<label class='l'><input type='radio' name='ENABLE' value='disabled' " + (!enabled ? String("checked") : String("")) + ">Disabled</label>\n";
    
      content += "<label class='l'><input type='radio' name='FIXED' value='sunrise' " + (useSunrise ? String("checked") : String("")) + ">Use Sunrise Time</label>\n";
      content += "<label class='l'><input type='radio' name='FIXED' value='fixed' " + (!useSunrise ? String("checked") : String("")) + ">Use Fixed Time</label>\n";
      content += "<div id='fixedWrap'><label>Fixed time: </label><input type='time' data-clear-btn='true' name='TIME' id='time' value='" + (hour < 24 ? String(hourbuf) + ":" : String("") ) + (minute < 60 ? String(minbuf) : String("") ) + "'></div>\n";
      //content += "<label>Fixed time:</label><input type='text' name='HOUR' placeholder='hour' class='h' value='" + (hour < 24 ? String(hour) : String("") ) + "'><input type='text' name='MINUTE' placeholder='min' class='h' value='" + (minute < 60 ? String(buffer) : String("") ) + "'>\n";
    
      content += "<label class='l'><input type='radio' name='MOONENABLE' value='enabled' " + (moonenabled ? String("checked") : String("")) + ">Moon Enabled</label>";
      content += "<label class='l'><input type='radio' name='MOONENABLE' value='disabled' " + (!moonenabled ? String("checked") : String("")) + ">Moon Disabled</label>";
      
      content += "<div id='fixedWrap'><label>Fixed time: </label><input type='time' data-clear-btn='true' name='TIME' id='time' value='" + (hour < 24 ? String(hourbuf) + ":" : String("") ) + (minute < 60 ? String(minbuf) : String("") ) + "'></div>";
      //content += "<label>Fixed time:</label><input type='text' name='HOUR' placeholder='hour' class='h' value='" + (hour < 24 ? String(hour) : String("") ) + "'><input type='text' name='MINUTE' placeholder='min' class='h' value='" + (minute < 60 ? String(buffer) : String("") ) + "'>";
    
      content += "<div><button name='SUNRISE' value='Sunrise'>Sunrise</button><button name='SUNSET' value='Sunset'>Sunset</button>";
      content += "<button name='SUBMIT' value='Save'>Save</button></div></form></div><div data-role='footer'><h3>" + _sunrise->GetState() + "</h3>";
      content += "<div class='pb' style='width:" + String(_sunrise->GetPercent()) + "%;background-color:#" + _sunrise->GetColor() + "'>&nbsp;</div>";
      content += "You can access this page until you <a href=\"/login?DISCONNECT=YES\">disconnect</a></div>" + javascript() + "</body></html>";
      server->sendHeader("Cache-Control", "no-cache");
      server->send(200, "text/html", content);
    }
    
    String javascript() {
      return "<script>$(document).ready(function() {\n\
      $.ajaxSetup({ cache:false });\n\
      setTimeout(repeater, " + String(LEDDELAY) + ");\n\
      $('input[type=radio][name=FIXED]').change(function() \n\
      { if ($('input[name=""FIXED""]:checked').val() == 'fixed') { $('#fixedWrap').show(); }\n\
      else \n\
      { $('#fixedWrap').hide(); } } ).change();});</script>\n";
    }
    
    //no need authentification
    void handleNotFound(){
      String message = "File Not Found\n\n";
      message += "URI: ";
      message += server->uri();
      message += "\nMethod: ";
      message += (server->method() == HTTP_GET)?"GET":"POST";
      message += "\nArguments: ";
      message += server->args();
      message += "\n";
      for (uint8_t i=0; i<server->args(); i++){
        message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
      }
      server->send(404, "text/plain", message);
    }
};
