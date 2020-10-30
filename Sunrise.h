#ifndef SUNRISE_H
#define SUNRISE_H
#include <Adafruit_NeoPixel.h>

class Sunrise {
  public: 
    typedef std::function<void(const char*)> THandlerFunction;
    Sunrise(int Delay, int FastDelay, int NumLeds, int pin, THandlerFunction stateChange);

    void StartSunrise();
    
    void StartSunset();
    
    void StartMoonrise();
    
    void StartMoonset();

    void SetPixel(int pixel, byte r, byte g, byte b);

    const char * GetState();

    float GetPercent();

    String GetColor();
    
    void Update();

    void On();

    void Off();

    void FastToggle();

    void Toggle();

    void SetValue(int r, int g, int b);
    
    unsigned long GetValue();

    void StripShow();
    
  protected:
  private:
    int _delay;
    int _fastDelay;
    int workingDelay;
    int numLeds;
    long startTime;
	long lastUpdateTime = 0;
    byte R = 0;
    byte G = 0;
    byte B = 0; 
	THandlerFunction _stateChange;

    bool showSunrise = false;
    bool showSunset = false;
    bool showMoon = false;
	bool stateChanged = false;

    Adafruit_NeoPixel *strip = NULL;

    void sunrise();
    void sunset();
    void moonrise();
    void moonset();
};
#endif
