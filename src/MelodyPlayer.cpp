// -------------------------------------------------
// Copyright (c) 2022 HiBit <https://www.hibit.dev>
// -------------------------------------------------

 #include "MelodyPlayer.h"
 
void melodyPlayerTaskFunction(void* pPlayer) {
    while (true)
        ((MelodyPlayer*)pPlayer)->Play(melodyDoom, durationsDoom, lengthDoom, 80);
}

MelodyPlayer::MelodyPlayer(char buzzer_pin_) {
    buzzer_pin = buzzer_pin_;
    pinMode(buzzer_pin, OUTPUT);

    TaskHandle_t melodyPlayerTaskHandle;
    xTaskCreatePinnedToCore(
        melodyPlayerTaskFunction,    /* Task function. */
        "melodyPlayerTask",          /* name of task. */
        8000,                        /* Stack size of task */
        this,                        /* parameter of the task */
        tskIDLE_PRIORITY,            /* priority of the task */
        &melodyPlayerTaskHandle,     /* Task handle to keep track of created task */
        0                            /* pin task to core 0 */
    );
}

void MelodyPlayer::Play(const int melody[], const int durations[], int melodyLength, int speed_perc/* = 100*/) {
    for (int note = 0; note < melodyLength; note++) {
        //to calculate the note duration, take one second divided by the note type.
        //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
        int duration;
        duration = speed_perc * 1000 / pgm_read_word(&durations[note]) / 100;
        //tone(buzzer_pin, pgm_read_word(&melody[note]), duration);
        //duration = 1000 / durations[note];
        //tone(buzzer_pin, melody[note], duration);

        //to distinguish the notes, set a minimum time between them.
        //the note's duration + 30% seems to work well:
        int pauseBetweenNotes = duration * 1.30;
        delay(pauseBetweenNotes);
    
        //stop the tone playing:
        //noTone(buzzer_pin);
    }
}

void MelodyPlayer::Beep() {
    Beep(NOTE_B6, 32);
}

void MelodyPlayer::Beep(int note, int duration) {
    duration = 1000 / duration;
    //tone(buzzer_pin, note, duration);
    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);
    //noTone(buzzer_pin);
}
