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

    Adafruit_NeoPixel strip;
    
    void sunrise() {
      if (R < 255)
        R++;
      if (R > 50 && G < 255)
        G++;
      if (R > 100 && B < 255)
        B++;
      for (int i = 0; i < numLeds; i++) {
        strip.setPixelColor(i, R, G, B);   
      }
    }
  
  void sunset() {
      if (R > 0 && G < 155)
        R--;
      if (G > 0 && B < 205)
        G--;
      if (B > 0)
        B--;
      for (int i = 0; i < numLeds; i++) {
        strip.setPixelColor(i, R, G, B);  
      }   
  }

  public: 
    Sunrise(int Delay, Adafruit_NeoPixel Strip, int NumLeds) {
      _delay = Delay;
      strip = Strip;
      numLeds = NumLeds;
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
    
    void Update() {
        unsigned long current = millis();
        if (current - startTime > _delay) {
          if (showSunrise)
            sunrise();
          else if (showSunset)
            sunset();
          strip.show();
          startTime = current;
        }
    }
};
