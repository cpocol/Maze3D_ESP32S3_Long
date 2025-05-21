#pragma once

/***********************config*************************/

#define SPI_FREQUENCY         32000000 // corruption occured at 40000000 so 32000000 is probably a safe upper limit
#define TFT_SPI_MODE          SPI_MODE0
#define TFT_SPI_HOST          SPI2_HOST

/***********************config*************************/

//#define TFT_WIDTH             180 //native portrait mode
//#define TFT_HEIGHT            640

//#ifdef TFT_WIDTH
//#define EXAMPLE_LCD_H_RES     TFT_WIDTH
//#else
//#define EXAMPLE_LCD_H_RES     180
//#endif
//#ifdef TFT_HEIGHT
//#define EXAMPLE_LCD_V_RES     TFT_HEIGHT
//#else
//#define EXAMPLE_LCD_V_RES     640
//#endif
//#define LVGL_LCD_BUF_SIZE     (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES)

#define SEND_BUF_SIZE         (28800/2)
#define TFT_QSPI_CS           12
#define TFT_QSPI_SCK          17
#define TFT_QSPI_D0           13
#define TFT_QSPI_D1           18
#define TFT_QSPI_D2           21
#define TFT_QSPI_D3           14
#define TFT_QSPI_RST          16
#define TFT_BL                1
#define PIN_BAT_VOLT          8
#define PIN_BUTTON_1          0
#define PIN_BUTTON_2          21

#define TOUCH_IICSCL 10
#define TOUCH_IICSDA 15
#define TOUCH_INT    11
#define TOUCH_RES    16
