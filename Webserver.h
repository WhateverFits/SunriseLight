#include <ESP8266WebServer.h>
#include <time.h>
#include "Sunrise.h"
#include "Config.h"

class Webserver {
  public:
    typedef std::function<void(void)> THandlerFunction;
    Webserver(int port, Sunrise *sunrise, THandlerFunction recalculate) {
      server = new ESP8266WebServer(port);
      _sunrise = sunrise;
    }

    void Initialize() {
      server->on("/", [this]() {
        handleRoot();
      });
      server->on("/login", [this]() {
        handleLogin();
      });
      server->on("/status", [this]() {
        handleStatus();
      });

      server->on("/debug", [this]() {
        handleDebug();
      });
      server->onNotFound([this]() {
        handleNotFound();
      });

      //here the list of headers to be recorded
      const char * headerkeys[] = {"User-Agent", "Cookie"} ;
      size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
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
    unsigned int authkey = -1;
    ESP8266WebServer *server = NULL;
    Sunrise *_sunrise = NULL;
    THandlerFunction _recalculate;


    //Check if header is present and correct
    bool is_authenticated() {
      if (authkey == -1) {
        EEPROM.get(AUTHKEYINDEX, authkey);
      }
      
      if (server->hasHeader("Cookie")) {
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
      <title> Sunrise Alarm Clock - " + String(DNSNAME) + "\n\
      </title>\n\
      </head>\n");
      //return String("<meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1, user-scalable=0'><style>form{width:80%;margin:0 auto;}label,input{display:inline-block;}label{width:40%;text-align:right;}label+input{width:30%;margin:0 20% 2% 4%;}.l{display:block;text-align:left;margin:0 auto 2% auto;width:60%;}.h{width:10%;margin-right:0px}</style>");
    }


    //login page, also called for disconnect
    void handleLogin() {
      String msg;
      if (server->hasHeader("Cookie")) {
        Serial.print("Found cookie: ");
        String cookie = server->header("Cookie");
        Serial.println(cookie);
      }
      if (server->hasArg("DISCONNECT")) {
        Serial.println("Disconnection");
        String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=0\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
        authkey = -1;
        server->sendContent(header);
        return;
      }
      if (server->hasArg("USERNAME") && server->hasArg("PASSWORD")) {
        if (server->arg("USERNAME") == "admin" &&  server->arg("PASSWORD") == "admin" ) {
          if (authkey == -1) {
            authkey = random(65535);
            EEPROM.put(AUTHKEYINDEX, authkey);
            EEPROM.commit();
          }
          String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=" + String(authkey) + ";Max-Age=604800\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
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

    String GetCompressedColor() {
      char buffer[7];

      unsigned long colors = _sunrise->GetValue();

      byte R = colors >> 16 & 0xFF;
      byte G = (colors >> 8) & 0xFF;
      byte B = colors & 0xFF;
      R = R * (255 - WEBCOLORCOMPRESS) / 255 + (R == 0 ? 0 : WEBCOLORCOMPRESS);
      G = G * (255 - WEBCOLORCOMPRESS) / 255 + (G == 0 ? 0 : WEBCOLORCOMPRESS);
      B = B * (255 - WEBCOLORCOMPRESS) / 255 + (B == 0 ? 0 : WEBCOLORCOMPRESS);
      sprintf(buffer, "%02x%02x%02x", R, G, B);
      //Serial.println(buffer);
      return String(buffer);
    }

    void handleStatus() {
      if (is_authenticated()) {
        server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server->sendHeader("Pragma", "no-cache");
        server->sendHeader("Expires", "0");
        String content = "{ \"percent\": " + String(_sunrise->GetPercent()) + ", \"color\": \"" + GetCompressedColor() + "\", \"state\": \"" + _sunrise->GetState() + "\" }";
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
        strftime(buffer, 80, "%I:%M%p\n", timeinfo);
        content += String(i) + " - " + String(buffer);
      }

      server->send(200, "text/plain", content);
    }

    //root page can be accessed only if authentification is ok
    void handleRoot() {
      Serial.println("Enter handleRoot");
      String header;
      if (!is_authenticated()) {
        String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
        server->sendContent(header);
        return;
      }
      String content = "<html>" + genCSS() + "<body data-role='page'><div data-role='header'><H2>Sunrise Alarm Control - " + String(DNSNAME) + "</H2></div><div class='ui-content'>";
      bool enabled = true;
      bool moonenabled = false;
      if (server->hasArg("ENABLE")) {
        enabled = server->arg("ENABLE") == "enabled";
        EEPROM.put(ENABLEDINDEX, enabled);
        EEPROM.commit();
      } else {
        EEPROM.get(ENABLEDINDEX, enabled);
      }

      if (server->hasArg("MOONENABLE")) {
        moonenabled = server->arg("MOONENABLE") == "enabled";
        EEPROM.put(MOONENABLEDINDEX, moonenabled);
        EEPROM.commit();
      } else {
        EEPROM.get(MOONENABLEDINDEX, moonenabled);
      }

      bool useSunrise = true;
      byte hour = 255;
      byte minute = 255;
      if (server->hasArg("FIXED")) {
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

      if (server->hasArg("SUNRISE") && server->arg("SUNRISE") == "Sunrise") {
        _sunrise->StartSunrise();
      }

      if (server->hasArg("SUNSET") && server->arg("SUNSET") == "Sunset") {
        _sunrise->StartSunset();
      }

      if (server->hasArg("ON") && server->arg("ON") == "On") {
        Serial.println("On requested");
        _sunrise->On();
      }

      if (server->hasArg("OFF") && server->arg("OFF") == "Off") {
        Serial.println("Off requested");
        _sunrise->Off();
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

      content += "<fieldset data-role='controlgroup' data-type='horizontal'>\n";
      content += "<label class='l'><input type='radio' name='ENABLE' value='enabled' " + (enabled ? String("checked") : String("")) + ">Enabled</label>\n";
      content += "<label class='l'><input type='radio' name='ENABLE' value='disabled' " + (!enabled ? String("checked") : String("")) + ">Disabled</label>\n";
      content += "</fieldset>\n";

      content += "<div id=\"allWrap\">\n";
      content += "<fieldset data-role='controlgroup' data-type='horizontal'>\n";
      content += "<label class='l'><input type='radio' name='MOONENABLE' value='enabled' " + (moonenabled ? String("checked") : String("")) + ">Moon Enabled</label>\n";
      content += "<label class='l'><input type='radio' name='MOONENABLE' value='disabled' " + (!moonenabled ? String("checked") : String("")) + ">Moon Disabled</label>\n";
      content += "</fieldset>\n";

      content += "<fieldset data-role='controlgroup' data-type='horizontal'>\n";
      content += "<label class='l'><input type='radio' name='FIXED' value='sunrise' " + (useSunrise ? String("checked") : String("")) + ">Use Sunrise Time</label>\n";
      content += "<label class='l'><input type='radio' name='FIXED' value='fixed' " + (!useSunrise ? String("checked") : String("")) + ">Use Fixed Time</label>\n";
      content += "</fieldset>\n";
      content += "<div id='fixedWrap'><label>Fixed time: </label><input type='time' data-clear-btn='true' name='TIME' id='time' value='" + (hour < 24 ? String(hourbuf) + ":" : String("") ) + (minute < 60 ? String(minbuf) : String("") ) + "'></div>\n";

      content += "<div><fieldset data-role=\"controlgroup\" data-type=\"horizontal\">\n";
      content += "<button name='SUNRISE' value='Sunrise' data-icon=\"arrow-u\">Sunrise</button><button name='SUNSET' value='Sunset' data-icon=\"arrow-d\">Sunset</button>\n";
      content += "<button name='ON' value='On' data-icon=\"plus\">On</button>\n";
      content += "<button name='OFF' value='Off' data-icon=\"minus\">Off</button>\n";
      content += "</fieldset>\n";
      content += "</div>\n</div>\n";
      content += "<button name='SUBMIT' value='Save' data-icon=\"check\">Save</button>\n";

      content += "</form></div><div data-role='footer'><h3>" + (String)_sunrise->GetState() + "</h3>\n";
      content += "<div class='pb' id='pb' style='width:" + String(_sunrise->GetPercent()) + "%;background-color:#" + GetCompressedColor() + "'>&nbsp;</div>\n";
      content += "You can access this page until you <a href=\"/login?DISCONNECT=YES\">disconnect</a></div>" + javascript() + "</body></html>\n";
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
      { $('#fixedWrap').hide(); } } ).change();\n\
      $('input[type=radio][name=ENABLE]').change(function() \n\
      { if ($('input[name=""ENABLE""]:checked').val() == 'enabled') { $('#allWrap').show(); }\n\
      else \n\
      { $('#allWrap').hide(); } } ).change();});</script>\n";
    }

    //no need authentification
    void handleNotFound() {
      String message = "File Not Found\n\n";
      message += "URI: ";
      message += server->uri();
      message += "\nMethod: ";
      message += (server->method() == HTTP_GET) ? "GET" : "POST";
      message += "\nArguments: ";
      message += server->args();
      message += "\n";
      for (uint8_t i = 0; i < server->args(); i++) {
        message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
      }
      server->send(404, "text/plain", message);
    }
};
