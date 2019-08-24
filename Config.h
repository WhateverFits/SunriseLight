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
#define LEDS 60
#define LOCALUDPPORT 8888
#define NEO_PIN D6
#define BUTTON_PIN D2
#define CLOCK_DIO D3
#define CLOCK_CLK D4


IPAddress timeServer(192, 168, 42, 1);
const char* ntpServerName = "us.pool.ntp.org";

const char* ssids[] = {"Acrid", "The Ranch-2.4", "ThunderLizard", "MakerHQ", "Milagro"};
const char* passs[] = {"MyVoiceIsMyPassport", "916-955-0942", "SnarfSnarf", "sacramentomaker916", "Milagro123"};
const int wifiCount = 5;

#endif
