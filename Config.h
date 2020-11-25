#ifndef _CONFIG
#define _CONFIG

#define ENABLEDINDEX 0
#define AUTHKEYINDEX sizeof(bool)
#define USESUNRISEINDEX AUTHKEYINDEX + sizeof(int)
#define FIXEDTIMEINDEX USESUNRISEINDEX + sizeof(bool)
#define MOONENABLEDINDEX FIXEDTIMEINDEX+ 2 * sizeof(byte)
#define DEBOUNCE 50
#define REPEATDELAY 1000
#define LEDDELAY 4000
#define FASTDELAY 10

#ifdef CONFAQUARIUM
#define LEDS 60
#define DNSNAME "aquarium"
#define CLOCKBRIGHT 1
#elif CONFBEDROOM
#define LEDS 60
#define DNSNAME "bedroomclock"
#define CLOCKBRIGHT 7
#elif CONFOFFICE
#define LEDS 60
#define DNSNAME "officeclock"
#define CLOCKBRIGHT 1
#else
#define LEDS 10
#define DNSNAME "testcircuit"
#define CLOCKBRIGHT 7
#endif

#define LOCALUDPPORT 8888
#define LED_PIN D0
#define NEO_PIN D7
#define BUTTON_PIN D3
#define CLOCK_DIO D5
#define CLOCK_CLK D6
#define WEBCOLORCOMPRESS 100
#define RTCSTALECOUNT 100
#define MQTT_SERVER "pi4"
#define MQTT_PORT 1883
#define MQTT_CHANNEL_PUB "home/" DNSNAME "/state"
#define MQTT_CHANNEL_SUB "home/" DNSNAME "/control"
#define MQTT_CHANNEL_LOG "home/" DNSNAME "/log"
#define MQTT_USER "clockuser"
#define MQTT_PASSWORD "clockuser"
#define UPDATE_URL "http://pi4/cgi-bin/test.rb"



// Try our router first to see if the timeserver is up on it.
IPAddress timeServer(192, 168, 1, 1);
//Then we try the pool.
const char* ntpServerName = "us.pool.ntp.org";

const char* ssids[] = {"Acrid", "The Ranch-2.4", "MakerHQ", "Milagro"};
const char* passs[] = {"MyVoiceIsMyPassport", "916-955-0942", "sacramentomaker916", "Milagro123"};
const int wifiCount = 4;
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message

#endif
