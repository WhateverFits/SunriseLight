#ifndef FORMATTING_H
#define FORMATTING_H
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


const char* formatTime(time_t t, char *tz)
{
  char buf[200];
  sprintf(buf, 
  	"%02d:%02d:%02d %d %s %04d %s", hour(t), minute(t), second(t), 
	day(t), monthShortStr(month(t)), year(t), tz);
  Serial.println(buf);
  return buf;
}

void printTime(time_t t, char *tz)
{
  Serial.print("The time is ");
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
#endif
