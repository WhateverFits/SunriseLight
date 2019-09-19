#include <Adafruit_NeoPixel.h>
#include "Sunrise.h"

void Sunrise::sunrise() {
  if (R < 255)
	R++;
  if (R > 20 && G < 255)
	G++;
  if (R > 100 && B < 255)
	B++;
  for (int i = 0; i < numLeds; i++) {
	if (R < numLeds && i >= R)
	  strip->setPixelColor(i, 0, 0, 0);
	else
	  strip->setPixelColor(i, R, G, B);
  }

  if (R < 255 ) {
	Serial.print("sunrise ");
	Serial.println(GetColor());
  }
}

void Sunrise::sunset() {
  if (R > 0 && G < 155)
	R--;
  if (G > 0 && B < 235)
	G--;
  if (B > 0)
	B--;
  for (int i = numLeds - 1; i >= 0; i--) {
	if (R < numLeds && i >= R)
	  strip->setPixelColor(i, 0, 0, 0);
	else
	  strip->setPixelColor(i, R, G, B);
  }   

  if (R > 0) {
	Serial.print("sunset ");
	Serial.println(GetColor());
  }
}

void Sunrise::moonrise() {
  if (B < 255)
	B++;
  if (B > 20 && G < 80)
	G++;
  if (B > 20 && R < 80)
	R++;
  for (int i = 0; i < numLeds; i++) {
	if (B < numLeds && i >= B)
	  strip->setPixelColor(i, 0, 0, 0);
	else
	  strip->setPixelColor(i, R, G, B);
  }

  Serial.print("moonrise ");
  Serial.println(GetColor());
}

void Sunrise::moonset() {
  if (B > 0 && G < 80)
	B--;
  if (G > 0)
	G--;
  if (R > 0)
	R--;
  for (int i = numLeds - 1; i >= 0; i--) {
	if (B < numLeds && i >= B)
	  strip->setPixelColor(i, 0, 0, 0);
	else
	  strip->setPixelColor(i, R, G, B);
  }

  if (R > 0) {
	Serial.print("moonset ");
	Serial.println(GetColor());
  }
}

void Sunrise::SetValue(int r, int g, int b) {
  R = r;
  G = g;
  B = b;

  for (int i = 0; i < numLeds; i++) {
    strip->setPixelColor(i, r, g, b);
  }

  strip->show();
}

unsigned long Sunrise::GetValue() {
  return R << 16 | G << 8 | B;
}

void Sunrise::Off() {
  SetValue(0, 0, 0);
  showSunrise = false;
  showSunset = false;
  showMoon = false;  
}

void Sunrise::On() {
  SetValue(255, 255, 255);
  showSunrise = true;
  showSunset = false;
  showMoon = false;  
}

void Sunrise::FastToggle() {
  if (showSunrise) {
	StartSunset();
  } else {
	StartSunrise();
  }

  workingDelay = _fastDelay;
}

Sunrise::Sunrise(int Delay, int FastDelay, int NumLeds, int pin) {
  _delay = Delay;
  _fastDelay = FastDelay;
	workingDelay = Delay;
  numLeds = NumLeds;
  strip = new Adafruit_NeoPixel(NumLeds, pin, NEO_GRB + NEO_KHZ800);
  strip->begin();
  strip->show();
}

void Sunrise::StartSunrise() {
  showSunrise = true;
  showSunset = false;
  showMoon = false;
  startTime = millis();
  workingDelay = _delay;
}

void Sunrise::StartSunset() {
  showSunrise = false;
  showSunset = true;
  showMoon = false;
  startTime = millis();
  workingDelay = _delay;
}

void Sunrise::StartMoonrise() {
  showSunrise = false;
  showSunset = false;
  showMoon = true;
  startTime = millis();
  workingDelay = _delay;
}

void Sunrise::StartMoonset() {
  showSunrise = false;
  showSunset = false;
  showMoon = false;
  startTime = millis();
  workingDelay = _delay;
}

void Sunrise::SetPixel(int pixel, byte r, byte g, byte b) {
  strip->setPixelColor(pixel, r, g, b);
}

String Sunrise::GetState() {
  if (showSunrise)
    return "Sunrise";
  else if (showSunset)
    return "Sunset";
  else if (showMoon)
    return "Moon";
  else
    return "Off";
}

float Sunrise::GetPercent() {
  return ((float)(R + G + B)) / (255.0 * 3.0) * 100.0;
}

String Sunrise::GetColor() {
  char buffer[7];
  sprintf(buffer, "%02x%02x%02x", R, G, B);
  return String(buffer);
}

void Sunrise::Update() {
  unsigned long current = millis();
  if (current - startTime > workingDelay || current < startTime) {
	if (showSunrise)
	  sunrise();
	else if (showSunset)
	  sunset();
	else if (showMoon)
	  moonrise();
	else
	  moonset();
	strip->show();
	startTime = current;
  }
}
