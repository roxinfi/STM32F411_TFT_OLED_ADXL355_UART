#include "ili9341.h"
#include "ili9341_font5x7.h"
#include <string.h>

/* ILI9341 commands */
#define ILI9341_SWRESET  0x01
#define ILI9341_SLPOUT   0x11
#define ILI9341_DISPON   0x29
#define ILI9341_CASET    0x2A
#define ILI9341_PASET    0x2B
#define ILI9341_RAMWR    0x2C
#define ILI9341_MADCTL   0x36
#define ILI9341_PIXFMT   0x3A

static inline void CS_LOW (ili9341_t *t){ HAL_GPIO_WritePin(t->cs_port, t->cs_pin, GPIO_PIN_RESET); }
static inline void CS_HIGH(ili9341_t *t){ HAL_GPIO_WritePin(t->cs_port, t->cs_pin, GPIO_PIN_SET);   }
static inline void DC_LOW (ili9341_t *t){ HAL_GPIO_WritePin(t->dc_port, t->dc_pin, GPIO_PIN_RESET); }
static inline void DC_HIGH(ili9341_t *t){ HAL_GPIO_WritePin(t->dc_port, t->dc_pin, GPIO_PIN_SET);   }

static void RST_Pulse(ili9341_t *t)
{
    if (!t->rst_port) return;
    HAL_GPIO_WritePin(t->rst_port, t->rst_pin, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(t->rst_port, t->rst_pin, GPIO_PIN_SET);
    HAL_Delay(120);
}

static HAL_StatusTypeDef spi_tx(ili9341_t *t, const uint8_t *data, uint16_t len)
{
    return HAL_SPI_Transmit(t->hspi, (uint8_t*)data, len, HAL_MAX_DELAY);
}

static void WriteCommand(ili9341_t *t, uint8_t cmd)
{
    CS_LOW(t);
    DC_LOW(t);
    spi_tx(t, &cmd, 1);
    CS_HIGH(t);
}

static void WriteData(ili9341_t *t, const uint8_t *data, uint16_t len)
{
    CS_LOW(t);
    DC_HIGH(t);
    spi_tx(t, data, len);
    CS_HIGH(t);
}

static void WriteData8(ili9341_t *t, uint8_t d)
{
    WriteData(t, &d, 1);
}

static void WriteData16(ili9341_t *t, uint16_t d)
{
    uint8_t b[2] = { (uint8_t)(d >> 8), (uint8_t)(d & 0xFF) };
    WriteData(t, b, 2);
}

static void SetAddrWindow(ili9341_t *t, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    WriteCommand(t, ILI9341_CASET);
    uint8_t caset[4] = { (uint8_t)(x0 >> 8), (uint8_t)x0, (uint8_t)(x1 >> 8), (uint8_t)x1 };
    WriteData(t, caset, 4);

    WriteCommand(t, ILI9341_PASET);
    uint8_t paset[4] = { (uint8_t)(y0 >> 8), (uint8_t)y0, (uint8_t)(y1 >> 8), (uint8_t)y1 };
    WriteData(t, paset, 4);

    WriteCommand(t, ILI9341_RAMWR);
}

static void WriteColorRepeat(ili9341_t *t, uint16_t color, uint32_t count)
{
    // Send repeated RGB565 color efficiently in small chunks
    uint8_t chunk[64]; // 32 pixels = 64 bytes
    for (int i = 0; i < 32; i++) {
        chunk[2*i]   = (uint8_t)(color >> 8);
        chunk[2*i+1] = (uint8_t)(color & 0xFF);
    }

    CS_LOW(t);
    DC_HIGH(t);
    while (count) {
        uint32_t px = (count > 32) ? 32 : count;
        HAL_SPI_Transmit(t->hspi, chunk, (uint16_t)(px * 2), HAL_MAX_DELAY);
        count -= px;
    }
    CS_HIGH(t);
}

HAL_StatusTypeDef ILI9341_Init(ili9341_t *tft)
{
    if (!tft || !tft->hspi) return HAL_ERROR;

    tft->width  = ILI9341_TFTWIDTH;
    tft->height = ILI9341_TFTHEIGHT;
    tft->rotation = 0;

    CS_HIGH(tft);
    DC_HIGH(tft);
    RST_Pulse(tft);

    WriteCommand(tft, ILI9341_SWRESET);
    HAL_Delay(150);

    // --- A practical init sequence that works on most ILI9341 SPI modules ---
    WriteCommand(tft, 0xCF); uint8_t d_cf[] = {0x00,0xC1,0x30}; WriteData(tft, d_cf, 3);
    WriteCommand(tft, 0xED); uint8_t d_ed[] = {0x64,0x03,0x12,0x81}; WriteData(tft, d_ed, 4);
    WriteCommand(tft, 0xE8); uint8_t d_e8[] = {0x85,0x00,0x78}; WriteData(tft, d_e8, 3);
    WriteCommand(tft, 0xCB); uint8_t d_cb[] = {0x39,0x2C,0x00,0x34,0x02}; WriteData(tft, d_cb, 5);
    WriteCommand(tft, 0xF7); WriteData8(tft, 0x20);
    WriteCommand(tft, 0xEA); uint8_t d_ea[] = {0x00,0x00}; WriteData(tft, d_ea, 2);

    WriteCommand(tft, 0xC0); WriteData8(tft, 0x23); // Power Control 1
    WriteCommand(tft, 0xC1); WriteData8(tft, 0x10); // Power Control 2
    WriteCommand(tft, 0xC5); uint8_t d_c5[] = {0x3E,0x28}; WriteData(tft, d_c5, 2); // VCOM
    WriteCommand(tft, 0xC7); WriteData8(tft, 0x86); // VCOM2

    WriteCommand(tft, ILI9341_MADCTL);
    WriteData8(tft, 0x48); // default rotation

    WriteCommand(tft, ILI9341_PIXFMT);
    WriteData8(tft, 0x55); // 16-bit/pixel (RGB565)

    WriteCommand(tft, 0xB1); uint8_t d_b1[] = {0x00,0x18}; WriteData(tft, d_b1, 2); // Frame rate
    WriteCommand(tft, 0xB6); uint8_t d_b6[] = {0x08,0x82,0x27}; WriteData(tft, d_b6, 3); // Display func

    WriteCommand(tft, 0xF2); WriteData8(tft, 0x00); // 3Gamma off
    WriteCommand(tft, 0x26); WriteData8(tft, 0x01); // Gamma curve

    // Positive gamma
    WriteCommand(tft, 0xE0);
    uint8_t gpos[] = {0x0F,0x31,0x2B,0x0C,0x0E,0x08,0x4E,0xF1,0x37,0x07,0x10,0x03,0x0E,0x09,0x00};
    WriteData(tft, gpos, sizeof(gpos));

    // Negative gamma
    WriteCommand(tft, 0xE1);
    uint8_t gneg[] = {0x00,0x0E,0x14,0x03,0x11,0x07,0x31,0xC1,0x48,0x08,0x0F,0x0C,0x31,0x36,0x0F};
    WriteData(tft, gneg, sizeof(gneg));

    WriteCommand(tft, ILI9341_SLPOUT);
    HAL_Delay(120);
    WriteCommand(tft, ILI9341_DISPON);
    HAL_Delay(20);

    ILI9341_SetRotation(tft, 0);
    ILI9341_FillScreen(tft, ILI9341_BLACK);
    return HAL_OK;
}

void ILI9341_SetRotation(ili9341_t *t, uint8_t r)
{
    t->rotation = r & 3;
    WriteCommand(t, ILI9341_MADCTL);

    // MADCTL bits: MY MX MV BGR
    switch (t->rotation) {
    default:
    case 0: WriteData8(t, 0x48); t->width=240; t->height=320; break;
    case 1: WriteData8(t, 0x28); t->width=320; t->height=240; break;
    case 2: WriteData8(t, 0x88); t->width=240; t->height=320; break;
    case 3: WriteData8(t, 0xE8); t->width=320; t->height=240; break;
    }
}

void ILI9341_DrawPixel(ili9341_t *t, uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= t->width || y >= t->height) return;
    SetAddrWindow(t, x, y, x, y);
    WriteData16(t, color);
}

void ILI9341_FillScreen(ili9341_t *t, uint16_t color)
{
    ILI9341_FillRect(t, 0, 0, t->width, t->height, color);
}

void ILI9341_FillRect(ili9341_t *t, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= t->width || y >= t->height) return;
    if (x + w > t->width)  w = t->width - x;
    if (y + h > t->height) h = t->height - y;

    SetAddrWindow(t, x, y, x + w - 1, y + h - 1);
    WriteColorRepeat(t, color, (uint32_t)w * h);
}

void ILI9341_DrawRect(ili9341_t *t, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    ILI9341_DrawLine(t, x, y, x+w-1, y, color);
    ILI9341_DrawLine(t, x, y+h-1, x+w-1, y+h-1, color);
    ILI9341_DrawLine(t, x, y, x, y+h-1, color);
    ILI9341_DrawLine(t, x+w-1, y, x+w-1, y+h-1, color);
}

static int iabs(int v){ return (v < 0) ? -v : v; }

void ILI9341_DrawLine(ili9341_t *t, int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx = iabs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
    int dy = -iabs(y1 - y0), sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (1) {
        if (x0 >= 0 && y0 >= 0) ILI9341_DrawPixel(t, (uint16_t)x0, (uint16_t)y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* Text */
void ILI9341_DrawChar(ili9341_t *t, uint16_t x, uint16_t y, char c,
                      uint16_t fg, uint16_t bg, uint8_t scale)
{
    if (scale == 0) scale = 1;
    if (c < 32 || c > 127) c = '?';

    const uint8_t *glyph = ILI9341_Font5x7[(uint8_t)c - 32];

    // Optional background box
    if (bg != fg) {
        ILI9341_FillRect(t, x, y, 6*scale, 8*scale, bg);
    }

    for (uint8_t col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (uint8_t row = 0; row < 8; row++) {
            if (bits & (1u << row)) {
                if (scale == 1) ILI9341_DrawPixel(t, x + col, y + row, fg);
                else ILI9341_FillRect(t, x + col*scale, y + row*scale, scale, scale, fg);
            }
        }
    }
}

void ILI9341_DrawString(ili9341_t *t, uint16_t x, uint16_t y, const char *s,
                        uint16_t fg, uint16_t bg, uint8_t scale)
{
    if (!s) return;
    uint16_t cx = x;

    while (*s) {
        if (*s == '\n') {
            cx = x;
            y += (uint16_t)(8 * scale + 1);
        } else {
            ILI9341_DrawChar(t, cx, y, *s, fg, bg, scale);
            cx += (uint16_t)(6 * scale);
        }
        s++;
    }
}

/* RGB565 bitmap */
void ILI9341_DrawRGB565(ili9341_t *t, uint16_t x, uint16_t y,
                        uint16_t w, uint16_t h, const uint16_t *pixels)
{
    if (!pixels) return;
    if (x >= t->width || y >= t->height) return;
    if (x + w > t->width)  w = t->width - x;
    if (y + h > t->height) h = t->height - y;

    SetAddrWindow(t, x, y, x + w - 1, y + h - 1);

    // Stream pixels (big endian over SPI)
    CS_LOW(t);
    DC_HIGH(t);
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        uint8_t b[2] = {(uint8_t)(pixels[i] >> 8), (uint8_t)pixels[i]};
        HAL_SPI_Transmit(t->hspi, b, 2, HAL_MAX_DELAY);
    }
    CS_HIGH(t);
}
