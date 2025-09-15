/*
  Build Options:
  Board  ESP32-S3-Dev
  USB CDC On boot Enabled
  Flash Size 16MB
  Partition Scheme 16M Flash(3MB APP/9.9MB FATFS)
  PSRAM "OPI PSRAM"
*/

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPowersLib.h>
#include <Wire.h>

#include "Config.h"
#include "Globals.h"
#include "Map.h"
#include "Textures.h"
#include "Controller.h"
#include "time.h"
#include "AXS15231B.h"
#include "MelodyPlayer.h"

#define DEBUG_COL (640 - 190)

PowersSY6970 PMU; //this helps turning off the annoying blinking LED

MelodyPlayer melodyPlayer(BUZZER_PIN);

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

uint16_t* screen = NULL;
uint16_t* background = new uint16_t[screenH];

const int16_t mapSizeHeight = mapHeight * sqRes, mapSizeWidth = mapWidth * sqRes;
const fptype mapSizeWidth_fp = (((fptype)mapSizeWidth) << fp), mapSizeHeight_fp = (((fptype)mapSizeHeight) << fp);

int32_t Tan_fp[around]; // fp bits fixed point
int32_t CTan_fp[around];

typedef struct {
    fptype xHit_fp, yHit_fp;
    int xMap, yMap;
} TCastResponse_fp;

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

int DiffAngles(int ang1, int ang2) { //suppose FOV is less than PI
    ang1 = (ang1 + 16 * around) % around;
    ang2 = (ang2 + 16 * around) % around;

    // when the two vectors are placed on both sides of the Ox axis
    if ((ang2 > around3q) && (ang1 < aroundq))
        return (around - ang2) + (ang1 - 0);
    else
    if ((ang1 > around3q) && (ang2 < aroundq))
        return -((around - ang1) + (ang2 - 0));
    else
        return ang1 - ang2;
}

unsigned short swapBytes(unsigned short value) {
    return (value % 256) * 256 + (value / 256);
}

bool result = false;
void setup()
{
    Serial.begin(115200); /* prepare for possible serial debug */
    delay(200); // just give Serial some time to start
    Serial.println("setup start");

    initController();

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);        // turn off backlight asap to minimise power on artifacts

    axs15231_init();
    lcd_setRotation(2);             // 180 degree hardware rotate if you want reset / boot buttons at the bottom
    lcd_fill(0, 0, 180, 640, 0x00); // clear screen
    digitalWrite(TFT_BL, HIGH);     // turn on backlight

    sprite.createSprite(180, 640);
    sprite.setRotation(3); //not working?
    sprite.setSwapBytes(1);
    screen = (uint16_t*)sprite.getPointer();

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

int CastX(int16_t angle, TCastResponse_fp responses[MAX_RESPONSES_XY]) { // hit vertical walls ||
    if ((angle == aroundq) || (angle == around3q)) {
        responses[0].xHit_fp = 1000000000;
        responses[0].yHit_fp = 1000000000;
        return 0; // CastY() will hit a wall correctly
    }

    int cnt = 0;
    TCastResponse_fp response;

    // prepare as for 1st or 4th quadrant (looking estward)
    response.xMap = (xC >> sqRes_pow2) + 1;
    int dxMap = 1,   adjXMap = 0;
    fptype dy_fp = sqRes * Tan_fp[angle];
    if ((aroundq < angle) && (angle < around3q)) { // 2nd or 3rd quadrant (looking weastward)
        response.xMap--;
        dxMap = -1;
        adjXMap = -1;
        dy_fp = -dy_fp;
    }

    response.yHit_fp = (((fptype)yC) << fp) + ((response.xMap << sqRes_pow2) - xC) * Tan_fp[angle];
    response.xMap += adjXMap;
    response.yMap = response.yHit_fp >> (fp + sqRes_pow2);

    while (cnt < MAX_RESPONSES_XY) {
        while ((0 < response.yHit_fp) && (response.yHit_fp < mapSizeHeight_fp) && //(0 < response.xMap) && (response.xMap < mapWidth) && //suppose the map is well closed
            (Map[response.yMap][response.xMap] == 0)) {
            response.xMap += dxMap;
            response.yHit_fp += dy_fp;
            response.yMap = response.yHit_fp >> (fp + sqRes_pow2);
        }
        response.xHit_fp = (fptype)(response.xMap - adjXMap) << (sqRes_pow2 + fp);

        responses[cnt++] = response;

        if (Map[response.yMap][response.xMap] < SPRITE) // opaque
            break;

        response.xMap += dxMap;
        response.yHit_fp += dy_fp;
        response.yMap = response.yHit_fp >> (fp + sqRes_pow2);
    }

    return cnt;
}

int CastY(int16_t angle, TCastResponse_fp responses[MAX_RESPONSES_XY]) { // hit horizontal walls ==
    if ((angle == 0) || (angle == aroundh)) {
        responses[0].xHit_fp = 1000000000;
        responses[0].yHit_fp = 1000000000;
        return 0; // CastX() will hit a wall correctly
    }

    int cnt = 0;
    TCastResponse_fp response;

    // prepare as for 1st or 2nd quadrant (lookog southward)
    response.yMap = (yC >> sqRes_pow2) + 1;
    int dyMap = 1,   adjYMap = 0;
    fptype dx_fp = sqRes * CTan_fp[angle];
    if (angle > aroundh) { // 3rd or 4th quadrants (looking northward)
        response.yMap--;
        dyMap = -1;
        adjYMap = -1;
        dx_fp = -dx_fp;
    }

    response.xHit_fp = (((fptype)xC) << fp) + ((response.yMap << sqRes_pow2) - yC) * CTan_fp[angle];
    response.yMap += adjYMap;
    response.xMap = response.xHit_fp >> (fp + sqRes_pow2);

    while (cnt < MAX_RESPONSES_XY) {
        while ((0 < response.xHit_fp) && (response.xHit_fp < mapSizeWidth_fp) && //(0 < response.yMap) && (response.yMap < mapHeight) && //suppose the map is well closed
            (Map[response.yMap][response.xMap] == 0)) {
            response.xHit_fp += dx_fp;
            response.yMap += dyMap;
            response.xMap = response.xHit_fp >> (fp + sqRes_pow2);
        }

        response.yHit_fp = (fptype)(response.yMap - adjYMap) << (sqRes_pow2 + fp);

        responses[cnt++] = response;

        if (Map[response.yMap][response.xMap] < SPRITE) // opaque
            break;

        response.xHit_fp += dx_fp;
        response.yMap += dyMap;
        response.xMap = response.xHit_fp >> (fp + sqRes_pow2);
     }

    return cnt;
}

int Cast(int angle, TCastResponse responses[MAX_RESPONSES]) {
    int cnt = 0;
    TCastResponse_fp responsesX[MAX_RESPONSES_XY], responsesY[MAX_RESPONSES_XY];
    int responsesCntX = CastX(angle, responsesX);
    int responsesCntY = CastY(angle, responsesY);

    // interleave responses
    int cntX = 0, cntY = 0;
    while ((cntX < responsesCntX) && (cntY < responsesCntY)) {
        if (llabs(((fptype)xC << fp) - responsesX[cntX].xHit_fp) < llabs(((fptype)xC << fp) - responsesY[cntY].xHit_fp)) { // vertical wall ||
            responses[cnt].xHit = int(responsesX[cntX].xHit_fp >> fp);
            responses[cnt].yHit = int(responsesX[cntX].yHit_fp >> fp);
            responses[cnt].xMap = responsesX[cntX].xMap;
            responses[cnt].yMap = responsesX[cntX].yMap;
            responses[cnt].horizontalWall = false;
            cntX++;
        }
        else { // horizontal wall ==
            responses[cnt].xHit = int(responsesY[cntY].xHit_fp >> fp);
            responses[cnt].yHit = int(responsesY[cntY].yHit_fp >> fp);
            responses[cnt].xMap = responsesY[cntY].xMap;
            responses[cnt].yMap = responsesY[cntY].yMap;
            responses[cnt].horizontalWall = true;
            cntY++;
        }
        cnt++;
    }
    while (cntX < responsesCntX) {
        responses[cnt].xHit = int(responsesX[cntX].xHit_fp >> fp);
        responses[cnt].yHit = int(responsesX[cntX].yHit_fp >> fp);
        responses[cnt].xMap = responsesX[cntX].xMap;
        responses[cnt].yMap = responsesX[cntX].yMap;
        responses[cnt].horizontalWall = false;
        cntX++;
        cnt++;
    }
    while (cntY < responsesCntY) {
        responses[cnt].xHit = int(responsesY[cntY].xHit_fp >> fp);
        responses[cnt].yHit = int(responsesY[cntY].yHit_fp >> fp);
        responses[cnt].xMap = responsesY[cntY].xMap;
        responses[cnt].yMap = responsesY[cntY].yMap;
        responses[cnt].horizontalWall = true;
        cntY++;
        cnt++;
    }

    // static int m = 0;
    // //if (n < 700) {
    //     if (col == DEBUG_COL && m == 0) {
    //         m++;
    //         Serial.println("Cast");
    //         for (int i = 0; i < cnt; i++)
    //             Serial.printf("%d, %d\n", responses[i].xHit, responses[i].yHit);
    //     }
    // }

    return cnt;
}

void RenderColumn(int col, int h, int textureColumn, TCastResponse response) {
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

    int8_t mapCell = Map[response.yMap][response.xMap];

    const uint16_t* pTexture;

    //special walls/sprites
    if (mapCell == SPRITE_TREE)
        pTexture = Sprite_PottedTree_SwappedBytes;
    else
    if (mapCell == SPRITE_BARREL)
        pTexture = Sprite_Barrel_SwappedBytes;
    else
    if (mapCell == SPRITE_LAMP)
        pTexture = Sprite_Lamp_SwappedBytes;
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
        pTexture = pTextures[response.horizontalWall]; // different texture for N/S walls
    }

    uint16_t* screenAddr = screen + col * screenH + minRow;
    const uint16_t* textureAddr = pTexture + textureColumn * texRes; // huge speedup: 90 degs pre-rotated texture; access it row wise (cache memory principles)

    if (mapCell >= SPRITE)
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
        TCastResponse responses[MAX_RESPONSES];
        int responsesCnt = Cast(ang, responses);

        int32_t h, textureColumn;

        int nearestOpaqueResponse = 0;
        for (int i = 0; i < responsesCnt; i++)
            if (Map[responses[i].yMap][responses[i].xMap] < SPRITE) {
                nearestOpaqueResponse = i;
                break;
            }

        //render responses from far to near
        for (int i = nearestOpaqueResponse; i >= 0; i--) {
            int8_t mapCell = Map[responses[i].yMap][responses[i].xMap];

            if (mapCell >= SPRITE) {
                int xSpriteCenter = responses[i].xMap * sqRes + sqResh;
                int ySpriteCenter = responses[i].yMap * sqRes + sqResh;
                int distSpriteCenter_sq = sq(xC - xSpriteCenter) + sq(yC - ySpriteCenter) + 1; // +1 avoids division by zero

                int angleSpriteCenter = (Rad2X(atan2f(ySpriteCenter - yC, xSpriteCenter - xC)) + around) % around;

                int deltaColSpriteCenter = DiffAngles(angleSpriteCenter, angleC);

                h = int(sqRes * sqrtf((viewerToScreen_sq + sq(deltaColSpriteCenter)) / (float)distSpriteCenter_sq));

                int w = h; // can be used interchangeably as the textures are square and the projective geometry is the same for both vertical and horizontal axes

                int deltaColSpriteColumn = DiffAngles(angleSpriteCenter, ang);

                textureColumn = deltaColSpriteColumn * texRes / w + texResh;
                if ((textureColumn < 0) || (textureColumn >= texRes))
                    continue;
            }
            else {
                int dist_sq = sq(xC - responses[i].xHit) + sq(yC - responses[i].yHit) + 1; // +1 avoids division by zero
                h = int(sqRes * sqrtf((viewerToScreen_sq + sq(screenWh - col)) / (float)dist_sq));

                textureColumn = ((responses[i].xHit + responses[i].yHit) % sqRes) * texRes / sqRes;
            }

            h = ADJ_HEIGHT(h); // need to lower it for wide screens
            RenderColumn(col, h, textureColumn, responses[i]);
        }
    }

    //sprite.drawLine(XY(640 - DEBUG_COL, 0), XY(640 - DEBUG_COL, 180), TFT_RED);
    renderController(sprite);

    auto t_render = millis();

    lcd_PushColors(screen, screenW * screenH);

    frameCnt++;

    auto t_show = millis();
    Serial.printf("Render: %2d ms     Send to screen: %2d ms     FPS: %.1f\n", t_render - t_start, t_show - t_render, 1000.f / (t_show - t_prev));
    
    t_prev = t_show;
}

void loop()
{
    Render();

    loopController(&xC, &yC, &angleC, around);

    //angleC = (angleC + 5) % around;
}
