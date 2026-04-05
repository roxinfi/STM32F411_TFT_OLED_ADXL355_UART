#pragma once
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define ILI9341_TFTWIDTH   240
#define ILI9341_TFTHEIGHT  320

// Common RGB565 colors
#define ILI9341_BLACK   0x0000
#define ILI9341_WHITE   0xFFFF
#define ILI9341_RED     0xF800
#define ILI9341_GREEN   0x07E0
#define ILI9341_BLUE    0x001F
#define ILI9341_YELLOW  0xFFE0
#define ILI9341_CYAN    0x07FF
#define ILI9341_MAGENTA 0xF81F
#define ILI9341_GRAY    0x8410

typedef struct {
    SPI_HandleTypeDef *hspi;

    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;

    GPIO_TypeDef *dc_port;
    uint16_t dc_pin;

    GPIO_TypeDef *rst_port;
    uint16_t rst_pin;

    uint16_t width;
    uint16_t height;
    uint8_t  rotation; // 0..3
} ili9341_t;

// Init + config
HAL_StatusTypeDef ILI9341_Init(ili9341_t *tft);
void ILI9341_SetRotation(ili9341_t *tft, uint8_t rotation);

// Drawing primitives
void ILI9341_DrawPixel(ili9341_t *tft, uint16_t x, uint16_t y, uint16_t color);
void ILI9341_FillScreen(ili9341_t *tft, uint16_t color);
void ILI9341_FillRect(ili9341_t *tft, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9341_DrawRect(ili9341_t *tft, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9341_DrawLine(ili9341_t *tft, int x0, int y0, int x1, int y1, uint16_t color);

// Text (5x7 font, ASCII 32..127) + scaling
void ILI9341_DrawChar(ili9341_t *tft, uint16_t x, uint16_t y, char c,
                      uint16_t fg, uint16_t bg, uint8_t scale);
void ILI9341_DrawString(ili9341_t *tft, uint16_t x, uint16_t y, const char *s,
                        uint16_t fg, uint16_t bg, uint8_t scale);

// Optional: draw raw RGB565 bitmap (w*h pixels)
void ILI9341_DrawRGB565(ili9341_t *tft, uint16_t x, uint16_t y,
                        uint16_t w, uint16_t h, const uint16_t *pixels);
