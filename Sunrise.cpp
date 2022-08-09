#include <Adafruit_NeoPixel.h>
#include <TimeLib.h>
#include "Sunrise.h"
#include "SunriseLight.h"

void Sunrise::sunrise() {
  if (R < _maxBrightness) {
    R++;
    stateChanged = true;
  }
  if (R > 90 && G < _maxBrightness) {
    G++;
    stateChanged = true;
  }
  if (R > 150 && B < _maxBrightness) {
    B++;
    stateChanged = true;
  }

  if (stateChanged) {
    for (int i = 0; i < numLeds; i++) {
      if (R < numLeds && i >= R * 4)
        strip->setPixelColor(i, 0, 0, 0);
      else
        strip->setPixelColor(i, R, G, B);
    }

    Serial.print("sunrise ");
    Serial.println(GetColor());
    if (_stateChange && R == _maxBrightness && G == _maxBrightness && B == _maxBrightness) {
      _stateChange(GetState());
      workingDelay = _delay;
    }
  }
}

void Sunrise::sunset() {
  if (R > 0 && G < 155) {
    R--;
    stateChanged = true;
  }
  if (G > 0 && B < 235) {
    G--;
    stateChanged = true;
  }
  if (B > 0) {
    B--;
    stateChanged = true;
  }
  for (int i = numLeds - 1; i >= 0; i--) {
    if (R < numLeds && i >= R * 4)
      strip->setPixelColor(i, 0, 0, 0);
    else
      strip->setPixelColor(i, R, G, B);
  }   

  if (stateChanged) {
    Serial.print("sunset ");
    Serial.println(GetColor());
    if (_stateChange && R == 0 && G == 0 && B == 0) {
      _stateChange(GetState());
      workingDelay = _delay;
    }
  }
}

void Sunrise::moonrise() {
  if (B < _maxBrightness) {
    B++;
    stateChanged = true;
  }
  if (B > 20 && G < 60) {
    G++;
    stateChanged = true;
  }
  if (B > 20 && R < 60) {
    R++;
    stateChanged = true;
  }
  for (int i = 0; i < numLeds; i++) {
    if (B < numLeds && i >= B * 4)
      strip->setPixelColor(i, 0, 0, 0);
    else
      strip->setPixelColor(i, R, G, B);
  }

  if (stateChanged) {
    Serial.print("moonrise ");
    Serial.println(GetColor());
    if (_stateChange && R == 80 && G == 80 && B == _maxBrightness) {
      _stateChange(GetState());
      workingDelay = _delay;
    }
  }
}

void Sunrise::moonset() {
  if (B > 0 && G < 60) {
    B--;
    stateChanged = true;
  }
  if (G > 0) {
    G--;
    stateChanged = true;
  }
  if (R > 0) {
    R--;
    stateChanged = true;
  }
  for (int i = numLeds - 1; i >= 0; i--) {
    if (B < numLeds && i >= B * 4)
      strip->setPixelColor(i, 0, 0, 0);
    else
      strip->setPixelColor(i, R, G, B);
  }

  if (stateChanged) {
    Serial.print("moonset ");
    Serial.println(GetColor());
    if (_stateChange && R == 0 && G == 0 && B == 0) {
      _stateChange(GetState());
      workingDelay = _delay;
    }
  }
}

void Sunrise::StripShow() {
  strip->show();
}

void Sunrise::SetValue(int r, int g, int b) {
  if (R != r || G != g || B != b) {
    stateChanged = true;
  }

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
  if (_stateChange) {
    _stateChange("Off");
  }
  stateChanged = true;
}

void Sunrise::On() {
  SetValue(_maxBrightness, _maxBrightness, _maxBrightness);
  showSunrise = true;
  showSunset = false;
  showMoon = false;  
  if (_stateChange) {
    _stateChange("On");
  }
  stateChanged = true;
}

bool Sunrise::FastToggle() {
  bool sunrising = false;
  if (showSunrise) {
    StartSunset();
    sunrising = false;
  } else {
    StartSunrise();
    sunrising = true;
  }

  workingDelay = _fastDelay;
  return sunrising;
}

bool Sunrise::Toggle() {
  float percent = GetPercent();
  bool sunrising = false;

  if (percent == 0) {
    StartSunrise();
    sunrising = true;
  } else if (percent == 100) {
    StartSunset();
    sunrising = false;
  } else if (showSunrise) {
    StartSunset();
    sunrising = false;
  } else {
    StartSunrise();
    sunrising = true;
  }

  return sunrising;
}

Sunrise::Sunrise(int Delay, int FastDelay, int NumLeds, int pin, int maxBrightness, THandlerFunction stateChange) {
  _delay = Delay;
  _fastDelay = FastDelay;
  _maxBrightness = maxBrightness;
  _stateChange = stateChange;
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
  if (_stateChange) {
    _stateChange("Sunrise");
  }
}

void Sunrise::StartSunset() {
  showSunrise = false;
  showSunset = true;
  showMoon = false;
  startTime = millis();
  workingDelay = _delay;
  if (_stateChange) {
    _stateChange("Sunset");
  }
}

void Sunrise::StartMoonrise() {
  showSunrise = false;
  showSunset = false;
  showMoon = true;
  startTime = millis();
  workingDelay = _delay;
  if (_stateChange) {
    _stateChange("Moonrise");
  }
}

void Sunrise::StartMoonset() {
  showSunrise = false;
  showSunset = false;
  showMoon = false;
  startTime = millis();
  workingDelay = _delay;
  if (_stateChange) {
    _stateChange("Moonset");
  }
}

void Sunrise::SetPixel(int pixel, byte r, byte g, byte b) {
  strip->setPixelColor(pixel, r, g, b);
}

const char * Sunrise::GetState() {
  if (GetPercent() == 100)
    return "On";
  else if (GetPercent() == 0)
    return "Off";
  else if (showSunrise)
    return "Sunrise";
  else if (showSunset)
    return "Sunset";
  else if (showMoon)
    return "Moon";
  else
    return "Off";
}

float Sunrise::GetPercent() {
  return ((float)(R + G + B)) / ((float)_maxBrightness * 3.0) * 100.0;
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
    if (_stateChange && stateChanged && (current >= lastUpdateTime + 1000 || current < lastUpdateTime)) {
      _stateChange(GetState());
      lastUpdateTime = current;
    }

    stateChanged = false;
  }
}
