#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel strip = Adafruit_NeoPixel(200, 7, NEO_GRB + NEO_KHZ800);

unsigned long lastButton[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned long startTime;
byte R = 0;
byte G = 0;
byte B = 0;
#define DEBOUNCE 150
#define DELAY 100
#define LEDS 200

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Setup start");

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}


bool debounceButton(int pin) {
  unsigned long now = millis();
  if (digitalRead(pin) == LOW && abs(now - lastButton[pin]) > DEBOUNCE) {
    lastButton[pin] = now;
    Serial.print("Button: ");
    Serial.println(pin);
    return true;
  } else {
    return false;
  }
}

void Sunrise(){
    if (R < 255)
      R++;
    if (R > 50 && G < 255)
      G++;
    if (R > 100 && B < 255)
      B++;
    for (int i = 0; i < LEDS; i++) {
      strip.setPixelColor(i, R, G, B);   
    }
}

void Sunset() {
    if (R > 0 && G < 155)
      R--;
    if (G > 0 && B < 205)
      G--;
    if (B > 0)
      B--;
    for (int i = 0; i < LEDS; i++) {
      strip.setPixelColor(i, R, G, B);  
    }   
}

void loop() {
  for (int i=0; i < 200; i++) {
    strip.setPixelColor(i, 50,50,50);
    strip.setPixelColor(i-1, 0,0,0);
    strip.show();
    delay(200);
  }
}

void loop3() {
  static long last = 0;
  static int i = 0;
  if (abs(millis() - last) > 200) {
    last = millis();
    strip.setPixelColor(i, 50,50,50); // Draw new pixel
    strip.setPixelColor(i-5, 0,0,0); // Erase pixel a few steps back
    strip.show();
    i++;
    if (i > 205)
      i = 0;
  }
}

void loop2() {
  static bool sunrise = true;
  static bool sunset = false;
  
   if (debounceButton(0)){
      if (!sunrise && !sunset) {
        sunrise = true;
      } else if (sunrise && !sunset) {
        sunrise = false;
        sunset = true;
      } else if (!sunrise && sunset) {
        sunrise = true;
        sunset = false;
      }

      startTime = millis();

      if (sunrise)
        Serial.println("sunrise");
      if (sunset)
        Serial.println("sunset");
   }
   unsigned long current = millis();
   if (current - startTime > DELAY) {
    if (sunrise)
      Sunrise();
    if (sunset)
      Sunset();
    strip.show();
    startTime = current;
   }

}
