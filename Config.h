#ifndef _CONFIG
#define _CONFIG

#define ENABLEDINDEX 0
#define AUTHKEYINDEX sizeof(bool)
#define USESUNRISEINDEX AUTHKEYINDEX + sizeof(int)
#define FIXEDTIMEINDEX USESUNRISEINDEX + sizeof(bool)
#define MOONENABLEDINDEX FIXEDTIMEINDEX+ 2 * sizeof(byte)
#define CLOCKBRIGHT 1
#define DEBOUNCE 50
#define REPEATDELAY 1000
#define LEDDELAY 4000
#define FASTDELAY 10
#define LEDS 124
#define LOCALUDPPORT 8888
#define LED_PIN D0
#define NEO_PIN D7
#define BUTTON_PIN D3
#define CLOCK_DIO D5
#define CLOCK_CLK D6
#define WEBCOLORCOMPRESS 100
#define RTCSTALECOUNT 100
#define DNSNAME "BEDROOMCLOCK"
#define MQTT_SERVER "pi4"
#define MQTT_PORT 1883
#define MQTT_CHANNEL_PUB "home/bedroomclock/state"
#define MQTT_CHANNEL_SUB "home/bedroomclock/control"
#define MQTT_USER "clockuser"
#define MQTT_PASSWORD "clockuser"



IPAddress timeServer(192, 168, 42, 88);
const char* ntpServerName = "us.pool.ntp.org";

const char* ssids[] = {""};
const char* passs[] = {""};
const int wifiCount = 4;

#endif
