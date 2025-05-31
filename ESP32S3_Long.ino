/*
  Build Options:
  Board  ESP32-S3-Dev
  USB CDC On boot Enabled
  Flash Size 16MB
  Partition Scheme 16M Flash(3MB APP/9.9MB FATFS)
  PSRAM "OPI PSRAM"
*/

#include <Arduino.h>
#include <XPowersLib.h>
#include <Wire.h>

#include "Config.h"
#include "Map.h"
#include "Textures.h"
#include "Controller.h"
#include "time.h"
#include "AXS15231B.h"
#include "MelodyPlayer.h"

PowersSY6970 PMU;

MelodyPlayer melodyPlayer(BUZZER_PIN);

uint16_t* screen = new uint16_t[screenW * screenH];
uint16_t* background = new uint16_t[screenH];

const int16_t mapSizeHeight = mapHeight * sqRes, mapSizeWidth = mapWidth * sqRes;
const fptype mapSizeWidth_fp = (((fptype)mapSizeWidth) << fp), mapSizeHeight_fp = (((fptype)mapSizeHeight) << fp);

int32_t Tan_fp[around]; // fp bits fixed point
int32_t CTan_fp[around];

int frameCnt = 0;

//initial
int xC = 2.5 * sqRes;
int yC = 2.5 * sqRes;
int angleC = 400;
int elevation_perc = 0; //as percentage from wall half height

float X2Rad(int X) {
    return (float)X * 3.1415f / aroundh;
}

int Rad2X(float rad) {
    return (int)(rad * aroundh / 3.1415f);
}

unsigned short swapBytes(unsigned short value) {
    return (value % 256) * 256 + (value / 256);
}

bool result = false;
void setup()
{
    Serial.begin(115200); /* prepare for possible serial debug */
    Serial.println("setup start");

    initController();

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);        // turn off backlight asap to minimise power on artifacts

    axs15231_init();
    lcd_setRotation(2);             // 180 degree hardware rotate if you want reset / boot buttons at the bottom
    lcd_fill(0,0,180,640,0x00);       // clear screen
    digitalWrite(TFT_BL, HIGH);       // turn on backlight

    Wire.begin(TOUCH_IICSDA, TOUCH_IICSCL);
    result =  PMU.init(Wire, TOUCH_IICSDA, TOUCH_IICSCL, SY6970_SLAVE_ADDRESS);
    if (result == false) {
        Serial.println("PMU is not online...");
        delay(50);
    } else {
        PMU.enableMeasure(); // To obtain voltage data, the ADC must be enabled first
        PMU.disableStatLed();
        PMU.disableOTG();
        // This LED light is the charging indicator for the SY6970. It will not light up when used solely with a battery.
        // When the battery is not connected and the device is powered by USB, the green LED will flash. During charging, the green LED will stay on continuously.
        // You can disable this LED function in the SY6970's registers.
    }

    int16_t i, j;
    // precalculate
    for (int16_t a = 0; a < around; a++) {
        float angf = X2Rad(a);

        // tangent (theoretical range is [-inf..+inf], in practice (-128..+128) is fine)
        fptype maxTan = (((fptype)128) << fp) - 1;
        float temp = tan(angf) * (1 << fp);
        Tan_fp[a] = (fptype)temp;
        if (temp > maxTan)
            Tan_fp[a] = maxTan;
        if (temp < -maxTan)
            Tan_fp[a] = -maxTan;

        // cotangent
        temp = 1 / tanf(angf) * (((fptype)1) << fp);
        CTan_fp[a] = (fptype)temp;
        if (temp > maxTan)
            CTan_fp[a] = maxTan;
        if (temp < -maxTan)
            CTan_fp[a] = -maxTan;
    }

    //prepare background (floor and ceiling)
    for (i = 0; i < screenHh; i++) {
        background[i] = swapBytes(0b0010000100000100); //ceiling
        background[screenHh + i] = swapBytes(0b0101001010001010); //floor
    }
}

//returns wall ID (as map position)
int CastX(int16_t angle, fptype& xHit_fp, fptype& yHit_fp) { // hit vertical walls ||
    if ((angle == aroundq) || (angle == around3q))
        return -1; // CastY() will hit a wall correctly

    // prepare as for 1st or 4th quadrant
    int x = (xC / sqRes) * sqRes + sqRes;
    int dx = sqRes,   adjXMap = 0;
    fptype dy_fp = sqRes * Tan_fp[angle];
    // 2nd or 3rd quadrant
    if ((aroundq < angle) && (angle < around3q)) {
        x -= sqRes;
        adjXMap = -1;
        dx = -dx;
        dy_fp = -dy_fp;
    }
    yHit_fp = (((fptype)yC) << fp) + (x - xC) * Tan_fp[angle];

    while ((0 < yHit_fp) && (yHit_fp < mapSizeHeight_fp) && //(0 < x) && (x < mapSizeWidth) && suppose the map is well closed
           (Map[(yHit_fp >> (fp + sqRes_pow2))][(x >> sqRes_pow2) + adjXMap] == 0)) {
        x += dx;
        yHit_fp += dy_fp;
    }

    xHit_fp = (fptype)x << fp;

    return int((yHit_fp / sqRes_fp) * mapWidth + (x / sqRes + adjXMap));
}

// returns wall ID (as map position)
int CastY(int16_t angle, fptype& xHit_fp, fptype& yHit_fp) { // hit horizontal walls ==
    if ((angle == 0) || (angle == aroundh))
        return -1; // CastX() will hit a wall correctly

    // prepare as for 1st or 2nd quadrant
    int y = (yC / sqRes) * sqRes + sqRes;
    int dy = sqRes,   adjYMap = 0;
    fptype dx_fp = sqRes * CTan_fp[angle];
    if (angle > aroundh) { // 3rd or 4th quadrants
        y -= sqRes;
        adjYMap = -1;
        dy = -dy;
        dx_fp = -dx_fp;
    }
    xHit_fp = (((fptype)xC) << fp) + (y - yC) * CTan_fp[angle];

    while ((0 < xHit_fp) && (xHit_fp < mapSizeWidth_fp) && //(0 < y) && (y < mapSizeHeight) && suppose the map is well closed
           (Map[(y >> sqRes_pow2) + adjYMap][(xHit_fp >> (fp + sqRes_pow2))] == 0)) {
        xHit_fp += dx_fp;
        y += dy;
    }

    yHit_fp = (fptype)y << fp;

    return int((y / sqRes + adjYMap) * mapWidth + (xHit_fp / sqRes_fp));
}

// returns wall ID (as map position and cell face)
int Cast(int angle, int& xHit, int& yHit) {
    fptype xX_fp = 1000000000, yX_fp = xX_fp, xY_fp = xX_fp, yY_fp = xX_fp;
    int wallIDX = CastX(angle, xX_fp, yX_fp);
    int wallIDY = CastY(angle, xY_fp, yY_fp);
    // choose the nearest hit point
    if (llabs(((fptype)xC << fp) - xX_fp) < llabs(((fptype)xC << fp) - xY_fp)) { // vertical wall ||
        xHit = int(xX_fp >> fp);
        yHit = int(yX_fp >> fp);
        return 2 * wallIDX + 0;
    }
    else { // horizontal wall ==
        xHit = int(xY_fp >> fp);
        yHit = int(yY_fp >> fp);
        return 2 * wallIDY + 1;
    }
}

void RenderColumn(int col, int h, int textureColumn, int wallID) {
// the initial version was modified in order to produce an image that's rotated with 90 degrees
// this way, there is no need to rotate it when flushing on the screen of T-Display S3 Long

    int32_t Dh_fp = (texRes << 22) / h; // 1 row in screen space is this many rows in texture space; use fixed point
    int32_t textureRow_fp = 0;
    //int minRow = screenHh - h / 2; // no elevation
    int minRow = ((100 - elevation_perc) * (2 * screenHh - h) / 2 + elevation_perc * screenHh) / 100;
    int maxRow = min(minRow + h, screenH);

    if (minRow < 0) { // clip
        textureRow_fp = -(minRow * Dh_fp);
        minRow = 0;
    }

    int mapPosition = wallID / 2;
    int8_t mapCell = *(&Map[0][0] + mapPosition);

    const uint16_t* pTexture;

    //special walls/sprites
    if (mapCell == TREE_SPRITE)
        pTexture = Sprite_PottedTree_SwappedBytes;
    else
    if (mapCell == DOOM_WALL) {
        static const uint16_t* pTextures[] = {Doom000_SwappedBytes, Doom001_SwappedBytes, Doom002_SwappedBytes, Doom003_SwappedBytes, Doom004_SwappedBytes, Doom005_SwappedBytes, Doom006_SwappedBytes, Doom007_SwappedBytes, Doom008_SwappedBytes, Doom009_SwappedBytes,
                                              Doom010_SwappedBytes, Doom011_SwappedBytes, Doom012_SwappedBytes, Doom013_SwappedBytes, Doom014_SwappedBytes, Doom015_SwappedBytes, Doom016_SwappedBytes, Doom017_SwappedBytes, Doom018_SwappedBytes, Doom019_SwappedBytes,
                                              Doom020_SwappedBytes, Doom021_SwappedBytes, Doom022_SwappedBytes, Doom023_SwappedBytes, Doom024_SwappedBytes, Doom025_SwappedBytes, Doom026_SwappedBytes, Doom027_SwappedBytes, Doom028_SwappedBytes, Doom029_SwappedBytes,
                                              Doom030_SwappedBytes, Doom031_SwappedBytes, Doom032_SwappedBytes, Doom033_SwappedBytes, Doom034_SwappedBytes, Doom035_SwappedBytes, Doom036_SwappedBytes, Doom037_SwappedBytes, Doom038_SwappedBytes, Doom039_SwappedBytes,
                                              Doom040_SwappedBytes, Doom041_SwappedBytes, Doom042_SwappedBytes, Doom043_SwappedBytes, Doom044_SwappedBytes, Doom045_SwappedBytes, Doom046_SwappedBytes, Doom047_SwappedBytes, Doom048_SwappedBytes, Doom049_SwappedBytes,
                                              Doom050_SwappedBytes, Doom051_SwappedBytes, Doom052_SwappedBytes, Doom053_SwappedBytes, Doom054_SwappedBytes, Doom055_SwappedBytes, Doom056_SwappedBytes, Doom057_SwappedBytes, Doom058_SwappedBytes, Doom059_SwappedBytes,
                                              Doom060_SwappedBytes, Doom061_SwappedBytes, Doom062_SwappedBytes, Doom063_SwappedBytes, Doom064_SwappedBytes, Doom065_SwappedBytes, Doom066_SwappedBytes, Doom067_SwappedBytes, Doom068_SwappedBytes, Doom069_SwappedBytes,

                                              Doom069_SwappedBytes, Doom068_SwappedBytes, Doom067_SwappedBytes, Doom066_SwappedBytes, Doom065_SwappedBytes, Doom064_SwappedBytes, Doom063_SwappedBytes, Doom062_SwappedBytes, Doom061_SwappedBytes, Doom060_SwappedBytes,
                                              Doom059_SwappedBytes, Doom058_SwappedBytes, Doom057_SwappedBytes, Doom056_SwappedBytes, Doom055_SwappedBytes, Doom054_SwappedBytes, Doom053_SwappedBytes, Doom052_SwappedBytes, Doom051_SwappedBytes, Doom050_SwappedBytes,
                                              Doom049_SwappedBytes, Doom048_SwappedBytes, Doom047_SwappedBytes, Doom046_SwappedBytes, Doom045_SwappedBytes, Doom044_SwappedBytes, Doom043_SwappedBytes, Doom042_SwappedBytes, Doom041_SwappedBytes, Doom040_SwappedBytes,
                                              Doom039_SwappedBytes, Doom038_SwappedBytes, Doom037_SwappedBytes, Doom036_SwappedBytes, Doom035_SwappedBytes, Doom034_SwappedBytes, Doom033_SwappedBytes, Doom032_SwappedBytes, Doom031_SwappedBytes, Doom030_SwappedBytes,
                                              Doom029_SwappedBytes, Doom028_SwappedBytes, Doom027_SwappedBytes, Doom026_SwappedBytes, Doom025_SwappedBytes, Doom024_SwappedBytes, Doom023_SwappedBytes, Doom022_SwappedBytes, Doom021_SwappedBytes, Doom020_SwappedBytes,
                                              Doom019_SwappedBytes, Doom018_SwappedBytes, Doom017_SwappedBytes, Doom016_SwappedBytes, Doom015_SwappedBytes, Doom014_SwappedBytes, Doom013_SwappedBytes, Doom012_SwappedBytes, Doom011_SwappedBytes, Doom010_SwappedBytes,
                                              Doom009_SwappedBytes, Doom008_SwappedBytes, Doom007_SwappedBytes, Doom006_SwappedBytes, Doom005_SwappedBytes, Doom004_SwappedBytes, Doom003_SwappedBytes, Doom002_SwappedBytes, Doom001_SwappedBytes, Doom000_SwappedBytes};
        static const int texturesNum = sizeof(pTextures) / sizeof(pTextures[0]);
        pTexture = pTextures[frameCnt % texturesNum];
    }
    else {
        static const uint16_t* pTextures[] = {WolfGRAY128x128rot_SwappedBytes, Wolf128x128rot_SwappedBytes};
        pTexture = pTextures[wallID & 1]; // different texture for N/S walls
    }

    uint16_t* screenAddr = screen + col * screenH + minRow;
    const uint16_t* textureAddr = pTexture + textureColumn * texRes; // huge speedup: 90 degs pre-rotated texture; access it row wise (cache memory principles)

    if (mapCell == TREE_SPRITE)
        for (int row = minRow; row < maxRow; row++, textureRow_fp += Dh_fp, screenAddr++) {
            uint16_t tex = *(textureAddr + (textureRow_fp >> 22));

            if (tex == 0xAF63)
                continue;

            *screenAddr = tex;
        }
    else
        for (int row = minRow; row < maxRow; row++, textureRow_fp += Dh_fp, screenAddr++) {
            uint16_t tex = *(textureAddr + (textureRow_fp >> 22));

            *screenAddr = tex;
        }
}

auto t_prev = millis();
void Render() {
    auto t_start = millis();

    for (int i = 0; i < screenW; i++)
        memcpy(screen + i * screenH, background, 2 * screenH);

    for (int16_t col = 0; col < screenW; col++) {
        int16_t ang = (angleC + screenWh - col + around) % around; //grows to the left of screen center
        int xHit, yHit;
        int wallID = Cast(ang, xHit, yHit);

        int32_t h, textureColumn;

        int mapPosition = wallID / 2;
        int8_t mapCell = *(&Map[0][0] + mapPosition);

        if (mapCell == TREE_SPRITE)
        {
            int xSpriteCenter = (mapPosition % mapWidth) * sqRes + sqResh;
            int ySpriteCenter = (mapPosition / mapWidth) * sqRes + sqResh;
            int distSpriteCenter_sq = sq(xC - xSpriteCenter) + sq(yC - ySpriteCenter) + 1; // +1 avoids division by zero

            int angleSpriteCenter = (Rad2X(atan2f(ySpriteCenter - yC, xSpriteCenter - xC)) + around) % around;

            int16_t deltaColSpriteCenter = angleC - angleSpriteCenter;

            h = int(sqRes * sqrt((viewerToScreen_sq + sq(deltaColSpriteCenter)) / (float)distSpriteCenter_sq));

            textureColumn = (screenWh - col) * texRes / h;
            textureColumn += 64 * texRes;
            textureColumn %= texRes;

            textureColumn = (angleSpriteCenter - ang) * texRes / h + texResh;
            if ((textureColumn < 0) || (textureColumn >= texRes))
                continue;
        }
        else
        {
            textureColumn = ((xHit + yHit) % sqRes) * texRes / sqRes;

            int dist_sq = sq(xC - xHit) + sq(yC - yHit) + 1; // +1 avoids division by zero
            h = int(sqRes * sqrt((viewerToScreen_sq + sq(screenWh - col)) / (float)dist_sq));
        }

        h = ADJ_HEIGHT(h); // need to lower it for wide screens
        RenderColumn(col, h, textureColumn, wallID);
    }

    auto t_render = millis();

    lcd_PushColors(screen, 2 * screenW * screenH);

    frameCnt++;

    auto t_show = millis();
    //Serial.printf("render: %2d ms,       show: %2d ms,       FPS: %.1f\n", t_render - t_start, t_show   - t_render, 1000.f / (t_show - t_prev));
    t_prev = t_show;
}

void loop()
{
    Render();

    loopController(&xC, &yC, &angleC, around);

    //angleC = (angleC + 5) % around;
}
