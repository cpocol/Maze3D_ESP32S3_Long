#pragma once

#define sq(x) ((x)*(x))
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

typedef struct {
    int16_t xHit, yHit;
    int16_t xMap, yMap;
    bool horizontalWall;
} TCastResponse;

#define MAX_RESPONSES_XY 5
#define MAX_RESPONSES (2 * MAX_RESPONSES_XY)

int Cast(int angle, TCastResponse responses[MAX_RESPONSES]);
