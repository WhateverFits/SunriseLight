
#include <Arduino.h>
#include <NeoPatterns.h>
#include "Config.h"

uint8 R = 0;
uint8 G = 0;
uint8 B = 0;
void MyUserPattern1(NeoPatterns *aNeoPatterns, color32_t aPixelColor, color32_t aBackgroundColor, uint16_t aIntervalMillis, uint8_t aDirection) {
    aNeoPatterns->Interval = aIntervalMillis;
    R = COLOR32_GET_RED(aNeoPatterns->Color1);
    G = COLOR32_GET_GREEN(aNeoPatterns->Color1);
    B = COLOR32_GET_BLUE(aNeoPatterns->Color1);
    aNeoPatterns->Direction = aDirection;
    aNeoPatterns->TotalStepCounter = MAXBRIGHTNESS + 156; // Number of colors in B plus where B starts in
    aNeoPatterns->ActivePattern = PATTERN_USER_PATTERN1;
}

bool UserPattern1Update(NeoPatterns *aNeoPatterns, bool aDoUpdate)
{
    if (aDoUpdate)
    {
        if (aNeoPatterns->decrementTotalStepCounterAndSetNextIndex())
        {
            Serial.println("Derped!");
            return true;
        }

        if (aNeoPatterns->Direction == DIRECTION_UP)
        {
            if (R < MAXBRIGHTNESS)
            {
                R++;
            }
            if (R > 90 && G < MAXBRIGHTNESS)
            {
                G++;
            }
            if (R > 150 && B < MAXBRIGHTNESS)
            {
                B++;
            }
            if (R >= MAXBRIGHTNESS && G >= MAXBRIGHTNESS && B >= MAXBRIGHTNESS)
            {
                aNeoPatterns->TotalStepCounter = 1;
            }
        }
        else
        {
            if (R > 0 && G < 155)
            {
                R--;
            }
            if (G > 0 && B < 235)
            {
                G--;
            }
            if (B > 0)
            {
                B--;
            }
            if (R == 0 && G == 0 && B == 0) {
                aNeoPatterns->TotalStepCounter = 1;
            }
        }

        for (int i = 0; i < aNeoPatterns->numPixels(); i++)
        {
            if (R < aNeoPatterns->numPixels() && i >= R)
                aNeoPatterns->setPixelColor(i, 0, 0, 0);
            else
                aNeoPatterns->setPixelColor(i, R, G, B);
        }

        aNeoPatterns->Color1 = COLOR32(R, G, B);
    }

    return false;
}

void MyUserPattern2(NeoPatterns *aNeoPatterns, color32_t aPixelColor, uint16_t aIntervalMillis, uint16_t repititions, uint8_t aDirection) {
    aNeoPatterns->Interval = aIntervalMillis;
    R = COLOR32_GET_RED(aNeoPatterns->Color1);
    G = COLOR32_GET_GREEN(aNeoPatterns->Color1);
    B = COLOR32_GET_BLUE(aNeoPatterns->Color1);
    aNeoPatterns->Direction = aDirection;
    aNeoPatterns->TotalStepCounter = MAXBRIGHTNESS + 1; // Number of colors in B plus where B starts in
    aNeoPatterns->ActivePattern = PATTERN_USER_PATTERN2;
}

bool UserPattern2Update(NeoPatterns *aNeoPatterns, bool aDoUpdate)
{
    bool complete = false;
    if (aDoUpdate)
    {
        if (aNeoPatterns->decrementTotalStepCounterAndSetNextIndex())
        {
            return true;
        }


        if (aNeoPatterns->Direction == DIRECTION_UP)
        {
            if (B < MAXBRIGHTNESS)
            {
                B++;
            }
            if (B > 20 && G < 60)
            {
                G++;
            }
            if (B > 20 && R < 60)
            {
                R++;
            }

            if (B >= MAXBRIGHTNESS)
            {
                complete = true;
                aNeoPatterns->TotalStepCounter = 1;
            }
        }
        else
        {
            if (B > 0 && G < 60)
            {
                B--;
            }
            if (G > 0)
            {
                G--;
            }
            if (R > 0)
            {
                R--;
            }

            if (R == 0 && G == 0 && B == 0) {
                complete = true;
                aNeoPatterns->TotalStepCounter = 1;
            }
        }
    }

    for (int i = 0; i < aNeoPatterns->numPixels(); i++)
    {
      if (R < aNeoPatterns->numPixels() && i >= B)
        aNeoPatterns->setPixelColor(i, 0, 0, 0);
      else
        aNeoPatterns->setPixelColor(i, R, G, B);
    }

    aNeoPatterns->Color1 = COLOR32(R,G,B);

    return complete;
}