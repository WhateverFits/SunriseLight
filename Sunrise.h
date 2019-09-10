#ifndef SUNRISE_H
#define SUNRISE_H
#include <Adafruit_NeoPixel.h>

class Sunrise {
  public: 
    Sunrise(int Delay, int FastDelay, int NumLeds, int pin);

    void StartSunrise();
    
    void StartSunset();
    
    void StartMoonrise();
    
    void StartMoonset();

    void SetPixel(int pixel, byte r, byte g, byte b);

    String GetState();

    float GetPercent();

    String GetColor();
    
    void Update();

    void On();

    void Off();

    void FastToggle();

    void SetValue(int r, int g, int b);
    
    unsigned long GetValue();
    
  protected:
    private:
    int _delay;
    int _fastDelay;
    int workingDelay;
    int numLeds;
    long startTime;
    byte R = 0;
    byte G = 0;
    byte B = 0; 

    bool showSunrise = false;
    bool showSunset = false;
    bool showMoon = false;

    Adafruit_NeoPixel *strip = NULL;

    void sunrise();
    void sunset();
    void moonrise();
    void moonset();
};
#endif
