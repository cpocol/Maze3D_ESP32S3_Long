#pragma once

#define sq(x) ((x)*(x))
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

int Cast(int ang, int& xHit, int& yHit);

extern const int8_t Map[16][14];
