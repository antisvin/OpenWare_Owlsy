/* vim: set ai et ts=4 sw=4: */
#ifndef __ILI9341_H__
#define __ILI9341_H__

#include "fonts.h"
#include <stdbool.h>
#include "device.h"
#include "gpio.h"

#define ILI9341_MADCTL_MY  0x80
#define ILI9341_MADCTL_MX  0x40
#define ILI9341_MADCTL_MV  0x20
#define ILI9341_MADCTL_ML  0x10
#define ILI9341_MADCTL_RGB 0x00
#define ILI9341_MADCTL_BGR 0x08
#define ILI9341_MADCTL_MH  0x04

// default orientation
#define ILI9341_WIDTH  240
#define ILI9341_HEIGHT 320
#define ILI9341_ROTATION (ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR)

// rotate right
/*
 #define ILI9341_WIDTH  320
 #define ILI9341_HEIGHT 240
 #define ILI9341_ROTATION (ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR)
 */

// rotate left
/*
 #define ILI9341_WIDTH  320
 #define ILI9341_HEIGHT 240
 #define ILI9341_ROTATION (ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR)
 */

// upside down
/*
 #define ILI9341_WIDTH  240
 #define ILI9341_HEIGHT 320
 #define ILI9341_ROTATION (ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR)
 */

/****************************/

//#include "gpio.h"

// Color definitions
//#define COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))


#ifdef __cplusplus
extern "C" {
#endif

// call before initializing any SPI devices

//#define ILI9341_Select() setPin(OLED_CS_GPIO_Port, OLED_CS_Pin)
//#define ILI9341_Unselect() clearPin(OLED_CS_GPIO_Port, OLED_CS_Pin)
#define ILI9341_Select() clearPin(OLED_CS_GPIO_Port, OLED_CS_Pin)
#define ILI9341_Unselect() setPin(OLED_CS_GPIO_Port, OLED_CS_Pin)


void ILI9341_Init(void);
HAL_StatusTypeDef ILI9341_ReadPowerMode(uint8_t buff[1]);
HAL_StatusTypeDef ILI9341_ReadDisplayStatus(uint8_t buff[5]);
HAL_StatusTypeDef ILI9341_ReadPixelFormat(uint8_t buff[1]);

HAL_StatusTypeDef ILI9341_SetAddressWindow(uint16_t y0, uint16_t y1);

#ifdef __cplusplus
}
#endif

#endif // __ILI9341_H__
