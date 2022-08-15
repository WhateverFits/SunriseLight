#ifndef SunriseLightHeader
#define SunriseLightHeader
void mqttLog(const char* status);

// MQTT method headers
void mqttPublish(const char *status);
void mqttCallback(char *topic, byte *payload, unsigned int length);

// Alarm header
void setupAlarms();


#endif
