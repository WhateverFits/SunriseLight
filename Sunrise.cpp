#include <TimeLib.h>
#include <NeoPatterns.hpp>
#include "Sunrise.h"
#include "SunriseLight.h"

Sunrise::Sunrise(int Delay, int FastDelay, int NumLeds, int pin, int MaxBrightness, THandlerFunction stateChange)
{
  _delay = Delay;
  _fastDelay = FastDelay;
  _stateChange = stateChange;
  _maxBrightness = MaxBrightness;
  workingDelay = Delay;
  numLeds = NumLeds;
  //strip = new NeoPatterns(numLeds, pin, NEO_GRB + NEO_KHZ800, &AnimComplete);
  strip = new NeoPatterns(numLeds, pin, NEO_GRB + NEO_KHZ800);
  strip->begin();
  strip->show();
}
/*
void Sunrise::AnimComplete(NeoPatterns *pattern)
{
  uint8_t tColor = 255;
  uint8_t tDuration = 255;
  pattern->Fade(pattern->LongValue1.Color2, NeoPixel::dimColorWithGamma32(NeoPatterns::Wheel(tColor), _maxBrightness), 128, tDuration);
  pattern->Fade(pattern->LongValue1.Color2, COLOR32_YELLOW, 128, tDuration);
  pattern->Fade(pattern->LongValue1.Color2, COLOR32_ORANGE, 128, tDuration);
  pattern->Fade(pattern->LongValue1.Color2, COLOR32_RED, 128, tDuration);
  pattern->Fade(pattern->LongValue1.Color2, COLOR32(1,0,0), 128, tDuration);
  pattern->ColorWipe(COLOR32_BLACK, tDuration, FLAG_DO_NOT_CLEAR, DIRECTION_DOWN);
  pattern->ActivePattern = PATTERN_NONE;
}*/



void Sunrise::StripShow()
{
  strip->show();
}

void Sunrise::SetValue(int r, int g, int b)
{
  strip->stop();
  strip->Color1 = COLOR32(r, g, b);
  for (int i = 0; i < numLeds; i++)
  {
    strip->setPixelColor(i, r, g, b);
  }

  strip->show();
}

unsigned long Sunrise::GetValue()
{
  return strip->Color1;
}

void Sunrise::Off()
{
  SetValue(0, 0, 0);
  if (_stateChange)
  {
    _stateChange("Off");
  }
  stateChanged = true;
}

void Sunrise::On()
{
  SetValue(_maxBrightness, _maxBrightness, _maxBrightness);
  if (_stateChange)
  {
    _stateChange("On");
  }
  stateChanged = true;
}

bool Sunrise::Toggle(bool fast)
{
  float percent = GetPercent();
  bool sunrising = false;

  if (percent == 0)
  {
    StartSunrise();
    sunrising = true;
  }
  else if (strip->ActivePattern == PATTERN_USER_PATTERN1 && strip->Direction == DIRECTION_UP)
  {
    StartSunset();
    sunrising = false;
  }
  else if (strip->ActivePattern == PATTERN_USER_PATTERN1 && strip->Direction == DIRECTION_DOWN)
  {
    StartSunrise();
    sunrising = true;
  }
  else if (strip->ActivePattern == PATTERN_USER_PATTERN2 && strip->Direction == DIRECTION_UP)
  {
    StartMoonset();
    sunrising = false;
  }
  else if (strip->ActivePattern == PATTERN_USER_PATTERN2 && strip->Direction == DIRECTION_DOWN)
  {
    StartMoonrise();
    sunrising = true;
  }
  else if (percent >= 1)
  {
    StartSunset();
    sunrising = false;
  }
  else
  {
    StartSunrise();
    sunrising = true;
  }

  return sunrising;
}

void Sunrise::StartSunrise(bool fast)
{
  UserPattern1(strip, 0, 0, fast ? _fastDelay : _delay, DIRECTION_UP);
  if (_stateChange)
  {
    _stateChange("Sunrise");
  }
}

void Sunrise::StartSunset(bool fast)
{
  workingDelay = _delay;
  UserPattern1(strip, 0, 0, workingDelay, DIRECTION_DOWN);
  if (_stateChange)
  {
    _stateChange("Sunset");
  }
}

void Sunrise::StartMoonrise(bool fast)
{
  workingDelay = _delay;
  UserPattern2(strip, 0, workingDelay, 0, DIRECTION_UP);
  if (_stateChange)
  {
    _stateChange("Moonrise");
  }
}

void Sunrise::StartMoonset(bool fast)
{
  workingDelay = _delay;
  UserPattern2(strip, 0, workingDelay, 0, DIRECTION_DOWN);
  if (_stateChange)
  {
    _stateChange("Moonset");
  }
}

void Sunrise::SetPixel(int pixel, byte r, byte g, byte b)
{
  strip->setPixelColor(pixel, r, g, b);
}

const char *Sunrise::GetState()
{
  if (GetPercent() == 100)
    return "On";
  else if (GetPercent() == 0)
    return "Off";
  else if (strip->ActivePattern == PATTERN_USER_PATTERN1 && strip->Direction == DIRECTION_UP)
    return "Sunrise";
  else if (strip->ActivePattern == PATTERN_USER_PATTERN1 && strip->Direction == DIRECTION_DOWN)
    return "Sunset";
  else if (strip->ActivePattern == PATTERN_USER_PATTERN2 && strip->Direction == DIRECTION_UP)
    return "MoonRise";
  else if (strip->ActivePattern == PATTERN_USER_PATTERN2 && strip->Direction == DIRECTION_DOWN)
    return "MoonSet";
  else
    return "Off";
}

float Sunrise::GetPercent()
{
  return ((float)( GET_RED(strip->Color1) + GET_GREEN(strip->Color1) + GET_BLUE(strip->Color1))) / ((float)_maxBrightness * 3.0) * 100.0;
}

String Sunrise::GetColor()
{
  char buffer[7];
  sprintf(buffer, "%02x%02x%02x", GET_RED(strip->Color1), GET_GREEN(strip->Color1), GET_BLUE(strip->Color1));
  return String(buffer);
}

void Sunrise::Update()
{
  strip->update();
}
