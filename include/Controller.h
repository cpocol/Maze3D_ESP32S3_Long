#pragma once
#include <TFT_eSPI.h>

void initController(void);
void loopController(int* x, int* y, int* angle, int around);
void renderController(TFT_eSprite& sprite);
