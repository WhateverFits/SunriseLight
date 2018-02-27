#ifndef _CONFIG
#define _CONFIG

#define ENABLEDINDEX 0
#define AUTHKEYINDEX sizeof(bool)
#define USESUNRISEINDEX AUTHKEYINDEX + sizeof(int)
#define FIXEDTIMEINDEX USESUNRISEINDEX + sizeof(bool)

IPAddress timeServer(192, 168, 80, 1);
const char* ntpServerName = "pool.ntp.org";

const char* ssids[] = {"Acrid", "The Ranch-2.4", "ThunderLizard", "MakerHQ"};
const char* passs[] = {"MyVoiceIsMyPassport", "916-955-0942", "SnarfSnarf", "sacramentomaker916"};
const int wifiCount = 4;

#endif
