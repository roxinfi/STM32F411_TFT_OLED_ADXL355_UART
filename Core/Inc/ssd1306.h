#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "ssd1306_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SSD1306_COLOR_BLACK = 0x00,
    SSD1306_COLOR_WHITE = 0x01,
    SSD1306_COLOR_INVERSE = 0x02
} ssd1306_color_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t currentX;
    uint16_t currentY;
    bool initialized;
    bool inverted;
} ssd1306_t;

// Public API
bool SSD1306_Init(void);
void SSD1306_UpdateScreen(void);

void SSD1306_Fill(ssd1306_color_t color);
void SSD1306_SetCursor(uint16_t x, uint16_t y);

bool SSD1306_WriteChar(char ch, ssd1306_color_t color);
bool SSD1306_WriteString(const char* str, ssd1306_color_t color);

void SSD1306_Debug_ResumeRAM(void);
void SSD1306_Debug_AllPixelsOn(void);


// Drawing
void SSD1306_DrawPixel(uint16_t x, uint16_t y, ssd1306_color_t color);
void SSD1306_DrawLine(int x0, int y0, int x1, int y1, ssd1306_color_t color);
void SSD1306_DrawRectangle(int x, int y, int w, int h, ssd1306_color_t color);
void SSD1306_FillRectangle(int x, int y, int w, int h, ssd1306_color_t color);
void SSD1306_DrawBitmap(int x, int y, const uint8_t* bitmap, int w, int h, ssd1306_color_t color);

// Optional display controls
void SSD1306_SetContrast(uint8_t contrast);
void SSD1306_SetDisplayOn(bool on);
void SSD1306_InvertDisplay(bool invert);

// Access to global instance (optional)
extern ssd1306_t ssd1306;

#ifdef __cplusplus
}
#endif
