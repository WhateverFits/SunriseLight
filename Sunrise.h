#include <Adafruit_NeoPixel.h>

class Sunrise {
  private:
    int _delay;
    int numLeds;
    long startTime;
    byte R = 0;
    byte G = 0;
    byte B = 0; 

    bool showSunrise = false;
    bool showSunset = false;
    bool showMoon = false;

    Adafruit_NeoPixel *strip = NULL;
    
    void sunrise() {
      if (R < 255)
        R++;
      if (R > 20 && G < 255)
        G++;
      if (R > 100 && B < 255)
        B++;
      for (int i = 0; i < numLeds; i++) {
        strip->setPixelColor(i, R, G, B);   
      }
    }
  
  void sunset() {
      if (R > 0 && G < 155)
        R--;
      if (G > 0 && B < 235)
        G--;
      if (B > 0)
        B--;
      for (int i = 0; i < numLeds; i++) {
        strip->setPixelColor(i, R, G, B);  
      }   
  }

  void moonrise() {
      if (B < 255)
        B++;
      if (B > 20 && G < 80)
        G++;
      if (B > 20 && R < 80)
        R++;
      for (int i = 0; i < numLeds; i++) {
        strip->setPixelColor(i, R, G, B);   
      }
    }

  void moonset() {
      if (B > 0 && G < 80)
        B--;
      if (G > 0)
        G--;
      if (R > 0)
        R--;
      for (int i = 0; i < numLeds; i++) {
        strip->setPixelColor(i, R, G, B);   
      }
    }

  public: 
    Sunrise(int Delay, int NumLeds, int pin) {
      _delay = Delay;
      numLeds = NumLeds;
      strip = new Adafruit_NeoPixel(NumLeds, pin, NEO_GRB + NEO_KHZ800);
      strip->begin();
      strip->show();
    }

    void StartSunrise() {
      showSunrise = true;
      showSunset = false;
      showMoon = false;
      startTime = millis();
    }
    
    void StartSunset() {
      showSunrise = false;
      showSunset = true;
      showMoon = false;
      startTime = millis();
    }
    
    void StartMoonrise() {
      showSunrise = false;
      showSunset = false;
      showMoon = true;
      startTime = millis();
    }
    
    void StartMoonset() {
      showSunrise = false;
      showSunset = false;
      showMoon = false;
      startTime = millis();
    }
    
    void Update() {
        unsigned long current = millis();
        if (current - startTime > _delay) {
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
};
