#ifndef SUNRISE_H
#define SUNRISE_H
//#include <Adafruit_NeoPixel.h>
#include <NeoPatterns.h>

class Sunrise
{
public:
  typedef std::function<void(const char *)> THandlerFunction;
  Sunrise(int Delay, int FastDelay, int NumLeds, int pin, int MaxBrightness, THandlerFunction stateChange);

  void StartSunrise(bool fast = false);

  void StartSunset(bool fast = false);

  void StartMoonrise(bool fast = false);

  void StartMoonset(bool fast = false);

  void SetPixel(int pixel, byte r, byte g, byte b);

  const char *GetState();

  float GetPercent();

  String GetColor();

  void Update();

  void On();

  void Off();

  bool Toggle(bool fast);

  void SetValue(int r, int g, int b);

  unsigned long GetValue();

  void StripShow();

  static void AnimComplete(NeoPatterns *aLedsPtr);
protected:
private:
  int _delay;
  int _fastDelay;
  int _maxBrightness;
  int workingDelay;
  int numLeds;
  long startTime;
  long lastUpdateTime = 0;
  THandlerFunction _stateChange;

  bool stateChanged = false;

  NeoPatterns *strip = NULL;

  void sunrise();
  void sunset();
  void moonrise();
  void moonset();
};
#endif
