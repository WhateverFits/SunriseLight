
#include <Arduino.h>
#include <NeoPatterns.h>
#include "Config.h"

uint8 R = 0;
uint8 G = 0;
uint8 B = 0;

void UserPattern1(NeoPatterns *aNeoPatterns, color32_t aPixelColor, color32_t aBackgroundColor, uint16_t aIntervalMillis, 
        uint8_t aDirection) {
    aNeoPatterns->ActivePattern = PATTERN_USER_PATTERN1;
    aNeoPatterns->Interval = aIntervalMillis;
    //aNeoPatterns->Color1 = aPixelColor;
    //aNeoPatterns->LongValue1.Color2 = abs(128 - tColor);
    R = GET_RED(aNeoPatterns->Color1);
    G = GET_GREEN(aNeoPatterns->Color1);
    B = GET_BLUE(aNeoPatterns->Color1);
    aNeoPatterns->LongValue1.BackgroundColor = aBackgroundColor;
    aNeoPatterns->Direction = aDirection;
    aNeoPatterns->TotalStepCounter = MAXBRIGHTNESS + 150; // Number of colors in B plus where B starts in
    aNeoPatterns->show();
    aNeoPatterns->lastUpdate = millis();
}

bool UserPattern1Update(NeoPatterns *aNeoPatterns, bool aDoUpdate)
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
                complete = true;
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
            if (R == 0 && G == 0 && B == 0)
                complete = true;
        }
    }

    for (int i = 0; i < aNeoPatterns->numPixels(); i++)
    {
      if (R < aNeoPatterns->numPixels() && i >= R)
        aNeoPatterns->setPixelColor(i, 0, 0, 0);
      else
        aNeoPatterns->setPixelColor(i, R, G, B);
    }

    aNeoPatterns->Color1 = COLOR32(R,G,B);

      //_stateChange(GetState());

    return complete;
}

void UserPattern2(NeoPatterns *aNeoPatterns, color32_t aPixelColor, uint16_t aIntervalMillis, uint16_t repititions,
        uint8_t aDirection) {
    //Serial.print("Pattern 1 Color:");Serial.println(aPixelColor);
    aNeoPatterns->ActivePattern = PATTERN_USER_PATTERN2;
    aNeoPatterns->Interval = aIntervalMillis;
    //aNeoPatterns->Color1 = aPixelColor;
    //aNeoPatterns->LongValue1.Color2 = abs(128 - tColor);
    R = GET_RED(aNeoPatterns->Color1);
    G = GET_GREEN(aNeoPatterns->Color1);
    B = GET_BLUE(aNeoPatterns->Color1);
    //aNeoPatterns->LongValue1.BackgroundColor = aBackgroundColor;
    aNeoPatterns->Direction = aDirection;
    aNeoPatterns->TotalStepCounter = MAXBRIGHTNESS; // Number of colors in B
    aNeoPatterns->show();
    aNeoPatterns->lastUpdate = millis();
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
                complete = true;
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

            if (R == 0 && G == 0 && B == 0)
                complete = true;
        }
    }

    for (int i = 0; i < aNeoPatterns->numPixels(); i++)
    {
      if (R < aNeoPatterns->numPixels() && i >= R * 4)
        aNeoPatterns->setPixelColor(i, 0, 0, 0);
      else
        aNeoPatterns->setPixelColor(i, R, G, B);
    }

      //_stateChange(GetState());

    aNeoPatterns->Color1 = COLOR32(R,G,B);

    return complete;
}