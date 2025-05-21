#include <Arduino.h>
#include <Wire.h>
#include "Config.h"
#include "Controller.h"
#include "Main.h"
#include "Map.h"
#include "AXS15231B.h"

// jump/crunch
int maxJumpHeight = (int)(1.05 * sqResh); // jump this high
int maxCrunchHeight = -(int)(0.7 * sqResh); // crunch this low
float fFPS = 30; // approximate FPS
int acceleratedMotion[200];
int maxJump_idx, maxCrunch_idx;
int verticalAdvance = 0;
int jumping = 0, crunching = 0;
int z = 0; // same unit as sqRes

void move(int* x, int* y, int angle) {
    float rad = angle * 6.2831f / around;
    int xTest = *x + (int)(MOVE_SPD * cos(rad));
    int yTest = *y + (int)(MOVE_SPD * sin(rad));

    // check for wall collision
    int safetyX = ((aroundq < angle) && (angle < around3q)) ? +safety_dist : -safety_dist;
    int safetyY = (angle < aroundh) ? -safety_dist : +safety_dist;
    int adjXMap = ((aroundq < angle) && (angle < around3q)) ? -1 : 0;
    int adjYMap = (angle > aroundh) ? -1 : 0;

    int xWall, yWall;
    int wallID = Cast(angle, xWall, yWall);
    if (sq(*x - xTest) + sq(*y - yTest) >= sq(*x - xWall) + sq(*y - yWall)) { // inside wall
        if (wallID % 2 == 0) { // vertical wall ||
            *x = xWall + safetyX;
            *y = yTest;                          //                __
            if (Map[*y / sqRes][*x / sqRes] != 0) // it's a corner |
                *y = (yTest / sqRes - adjYMap) * sqRes + safetyY;
        }
        else { // horizontal wall ==
            *x = xTest;
            *y = yWall + safetyY;                //                __
            if (Map[*y / sqRes][*x / sqRes] != 0) // it's a corner |
                *x = (xTest / sqRes - adjXMap) * sqRes + safetyX;
        }
    }
    else // free cell
        *x = xTest, *y = yTest;
}

void rotate(int* angle, int dir, int around) {
    *angle = (*angle + dir * ROTATE_SPD + around) % around;
}

void initController() {
    float fDist = 0, fSpeed = 0, G = 10.f * sqRes; // G was empirically chosen as we don't have a proper world scale here
    int i;
    for (i = 0; i < 200; i++) {
        acceleratedMotion[i] = (int)fDist;

        fSpeed += G / fFPS;
        fDist += fSpeed / fFPS;
    }

    // search for the acceleratedMotion entry so that we'll decelerate to zero speed at max jump height
    for (maxJump_idx = 0; maxJump_idx < 200; maxJump_idx++)
        if (acceleratedMotion[maxJump_idx] > maxJumpHeight)
            break;

    if (maxJump_idx >= 200) maxJump_idx = 199;

    elevation_perc = 100 * z / sqResh; // as percentage from wall half height

    pinMode(TOUCH_RES, OUTPUT);
    digitalWrite(TOUCH_RES, HIGH); delay(2);
    digitalWrite(TOUCH_RES, LOW); delay(10);
    digitalWrite(TOUCH_RES, HIGH); delay(2);
}

void loopController(int* x, int* y, int* angle, int around) {
    static int jump_idx;
    int bJump = 0, bCrunch = 0;

    static short tx, ty;

    static uint8_t ALS_ADDRESS = 0x3B;
    static uint8_t read_touchpad_cmd[11] = {0xb5, 0xab, 0xa5, 0x5a, 0x0, 0x0, 0x0, 0x8};

    uint8_t buff[20] = {0};
    Wire.beginTransmission(ALS_ADDRESS);
    Wire.write(read_touchpad_cmd, 8);
    Wire.endTransmission();
    Wire.requestFrom(int(ALS_ADDRESS), int(AXS_TOUCH_TWO_POINT_LEN));
    while (!Wire.available());
    Wire.readBytes(buff, AXS_TOUCH_TWO_POINT_LEN);

    uint8_t num = AXS_GET_POINT_NUM(buff);
    uint16_t type = AXS_GET_GESTURE_TYPE(buff);

    if (num && !type)
    {
        Serial.printf("num:%d", num);
        for (int i = 0; i < num; ++i) {
            Serial.print("     x"); Serial.print(i); Serial.print(": "); Serial.print(AXS_GET_POINT_X(buff, i));
            Serial.print("     y"); Serial.print(i); Serial.print(": "); Serial.print(AXS_GET_POINT_Y(buff, i));
        }
        Serial.println();
        
        tx = AXS_GET_POINT_X(buff, 0);
        ty = AXS_GET_POINT_Y(buff, 0);
        if (tx > screenW * 9 / 10) // left third of the screen
        {
            if (ty > screenH * 2 / 3) // show map
            ;
            else if (ty < screenH / 3) // strafe left
                move(x, y, (*angle - around / 4 + around) % around);
            else // rotate left
                rotate(angle, -ROTATE_SPD, around);
        }
        else
        if (tx < screenW / 10) // right third of the screen
        {
            if (ty > screenH * 2 / 3) // crunch
                bCrunch = 1;
            else if (ty < screenH / 3) // strafe right
                move(x, y, (*angle + around / 4 + around) % around);
            else // rotate right
                rotate(angle, +ROTATE_SPD, around);
        }
        else // center third of the screen
        {
            if (ty > screenH * 2 / 3) // jump
                bJump = 1;
            else if (ty < screenH / 3) // pedal backward
                move(x, y, (*angle + around / 2) % around);
            else // pedal forward
                move(x, y, *angle);
        }
    }

    // jump
    if (bJump && !jumping && !crunching) {
        jumping = 1;
        verticalAdvance = 1;
        jump_idx = maxJump_idx - 1;
        z = maxJumpHeight - acceleratedMotion[jump_idx];
    }
    else
    if (jumping) {
        if (verticalAdvance > 0) {
            if (jump_idx > 0) {
                jump_idx--;
                z = maxJumpHeight - acceleratedMotion[jump_idx];
            }
            else {
                verticalAdvance = -1;
                z = maxJumpHeight;
            }
        }
        else
        if (verticalAdvance < 0) {
            if (z > 0) {
                jump_idx++;
                z = max(0, maxJumpHeight - acceleratedMotion[jump_idx]);
            }
            else {
                verticalAdvance = 0;
                jumping = 0;
            }
        }
    }

    // crunch
    if (bCrunch && !jumping) {
        crunching = 1;
        if (z > maxCrunchHeight) {
            z -= VERTICAL_SPD;
            if (z < maxCrunchHeight)
                z = maxCrunchHeight;
        }
    }
    else
    if (crunching) {
        z += VERTICAL_SPD;
        if (z >= 0) {
            z = 0;
            crunching = 0;
        }
    }
    
    elevation_perc = 100 * z / sqResh; // as percentage from wall half height
}
