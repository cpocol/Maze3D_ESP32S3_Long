#pragma once
// -------------------------------------------------
// Copyright (c) 2021 HiBit <https://www.hibit.dev>
// -------------------------------------------------

#include <Arduino.h>
#include "Melodies.h"

class MelodyPlayer
{
public:
    MelodyPlayer(char buzzer_pin_);
    void Play(const int melody[], const int durations[], int melodyLength, int speed_perc = 100);
    void Beep();
    void Beep(int note, int duration);
private:
    char buzzer_pin;
};
