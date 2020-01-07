#include <stdint.h>
#include <RTClib.h>

RTC_DS3231 rtc;

/*-------- NTP code ----------*/
WiFiUDP Udp;

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
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

int ntpRetryCount = 3;
int ntpRetry = 0;
long rtcCount = RTCSTALECOUNT;
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


  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  // Fall back to using the global NTP pool server in case we cannot connect to internal NTP server
  if (ntpRetry > 1)
    WiFi.hostByName(ntpServerName, timeServer);
  Serial.print("Transmit NTP Request to ");
  Serial.println(IpAddress2String(timeServer));
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
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
      setSyncInterval(300);
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
  Serial.println("No NTP Response :-(");
  if (ntpRetry < ntpRetryCount) 
  {
    ntpRetry++;
    return getNtpTime();
  }

  // We couldn't connect so we are gonna try harder!
  setSyncInterval(5);
  if (!rtc.lostPower()) {
	  return rtc.now().unixtime(); // return now if unable to get the time so we just get our RTC's time.
  } else {
	  return now(); // return now if unable to get the time so we just get our drifted internal time instead of wrong time.
  }
}
