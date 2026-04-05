#include "ssd1306.h"
#include "font5x7.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdlib.h>

extern I2C_HandleTypeDef SSD1306_I2C_HANDLE;

ssd1306_t ssd1306 = {
    .width = SSD1306_WIDTH,
    .height = SSD1306_HEIGHT,
    .currentX = 0,
    .currentY = 0,
    .initialized = false,
    .inverted = false
};

// Framebuffer: 1 bit per pixel
static uint8_t ssd1306_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

// ---------- Low-level I2C ----------
static bool ssd1306_write_command(uint8_t cmd) {
    // Control byte 0x00 = command
    uint8_t data[2] = {0x00, cmd};
    return (HAL_I2C_Master_Transmit(&SSD1306_I2C_HANDLE, SSD1306_I2C_ADDR, data, 2, SSD1306_I2C_TIMEOUT_MS) == HAL_OK);
}

static bool ssd1306_write_data(const uint8_t* bytes, uint16_t len) {
    // Control byte 0x40 = data
    // Send in chunks: [0x40][chunk...]
    uint8_t pkt[1 + SSD1306_I2C_CHUNK];
    pkt[0] = 0x40;

    while (len) {
        uint16_t n = (len > SSD1306_I2C_CHUNK) ? SSD1306_I2C_CHUNK : len;
        memcpy(&pkt[1], bytes, n);
        if (HAL_I2C_Master_Transmit(&SSD1306_I2C_HANDLE, SSD1306_I2C_ADDR, pkt, (uint16_t)(1 + n), SSD1306_I2C_TIMEOUT_MS) != HAL_OK) {
            return false;
        }
        bytes += n;
        len -= n;
    }
    return true;
}

// ---------- SSD1306 core ----------
static void ssd1306_reset_state(void) {
    ssd1306.currentX = 0;
    ssd1306.currentY = 0;
    ssd1306.inverted = false;
}

bool SSD1306_Init(void) {
    // Basic sanity
    if (SSD1306_WIDTH != 128) return false;
    if (SSD1306_HEIGHT != 32 && SSD1306_HEIGHT != 64) return false;

    HAL_Delay(50);

    // Init sequence (common, works for most modules)
    if (!ssd1306_write_command(0xAE)) return false; // Display OFF

    if (!ssd1306_write_command(0xD5)) return false; // Set display clock divide
    if (!ssd1306_write_command(0x80)) return false;

    if (!ssd1306_write_command(0xA8)) return false; // Set multiplex
    if (!ssd1306_write_command((SSD1306_HEIGHT == 64) ? 0x3F : 0x1F)) return false;

    if (!ssd1306_write_command(0xD3)) return false; // Set display offset
    if (!ssd1306_write_command(0x00)) return false;

    if (!ssd1306_write_command(0x40)) return false; // Set start line = 0

    if (!ssd1306_write_command(0x8D)) return false; // Charge pump
    if (!ssd1306_write_command(0x14)) return false;

    if (!ssd1306_write_command(0x20)) return false; // Memory addressing mode
    if (!ssd1306_write_command(0x00)) return false; // 0x00 = horizontal addressing

    if (!ssd1306_write_command(0xA1)) return false; // Segment remap
    if (!ssd1306_write_command(0xC8)) return false; // COM scan direction (remapped)

    if (!ssd1306_write_command(0xDA)) return false; // COM pins hw config
    if (!ssd1306_write_command((SSD1306_HEIGHT == 64) ? 0x12 : 0x02)) return false;

    if (!ssd1306_write_command(0x81)) return false; // Contrast
    if (!ssd1306_write_command(0x7F)) return false;

    if (!ssd1306_write_command(0xD9)) return false; // Pre-charge
    if (!ssd1306_write_command(0xF1)) return false;

    if (!ssd1306_write_command(0xDB)) return false; // VCOM detect
    if (!ssd1306_write_command(0x40)) return false;

    if (!ssd1306_write_command(0xA4)) return false; // Entire display ON (resume RAM)
    if (!ssd1306_write_command(0xA6)) return false; // Normal display (not inverted)

    if (!ssd1306_write_command(0x2E)) return false; // Deactivate scroll

    if (!ssd1306_write_command(0xAF)) return false; // Display ON

    ssd1306_reset_state();
    SSD1306_Fill(SSD1306_COLOR_BLACK);
    SSD1306_UpdateScreen();

    ssd1306.initialized = true;
    return true;
}

void SSD1306_UpdateScreen(void) {
	// Set column + page address and push framebuffer
	    uint8_t pages = SSD1306_HEIGHT / 8;

	    for (uint8_t page = 0; page < pages; page++) {
	        ssd1306_write_command((uint8_t)(0xB0 + page)); // page address
	        ssd1306_write_command(0x00);                   // lower column
	        ssd1306_write_command(0x10);                   // higher column
	        ssd1306_write_data(&ssd1306_buffer[SSD1306_WIDTH * page], SSD1306_WIDTH);
	    }

}

void SSD1306_Fill(ssd1306_color_t color) {
    uint8_t fill = 0x00;
    if (color == SSD1306_COLOR_WHITE) fill = 0xFF;
    if (color == SSD1306_COLOR_BLACK) fill = 0x00;

    memset(ssd1306_buffer, fill, sizeof(ssd1306_buffer));
}

void SSD1306_SetCursor(uint16_t x, uint16_t y) {
    ssd1306.currentX = x;
    ssd1306.currentY = y;
}

void SSD1306_DrawPixel(uint16_t x, uint16_t y, ssd1306_color_t color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;

    uint32_t index = x + (y / 8) * SSD1306_WIDTH;
    uint8_t  bit   = (uint8_t)(1U << (y % 8));

    switch (color) {
        case SSD1306_COLOR_WHITE:   ssd1306_buffer[index] |= bit;  break;
        case SSD1306_COLOR_BLACK:   ssd1306_buffer[index] &= ~bit; break;
        case SSD1306_COLOR_INVERSE: ssd1306_buffer[index] ^= bit;  break;
        default: break;
    }
}

bool SSD1306_WriteChar(char ch, ssd1306_color_t color) {
    if (ch < 0x20 || ch > 0x7F) ch = '?';

    // Each char is 5x7 + 1 column spacing
    if ((ssd1306.currentX + 6) >= SSD1306_WIDTH || (ssd1306.currentY + 8) >= SSD1306_HEIGHT) {
        return false;
    }

    const uint8_t* glyph = Font5x7[(uint8_t)ch - 0x20];

    // Draw 5 columns
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = glyph[col];
        for (uint8_t row = 0; row < 8; row++) { // 7 rows used; 8th keeps spacing
            if (line & 0x01) {
                SSD1306_DrawPixel(ssd1306.currentX + col, ssd1306.currentY + row, color);
            } else {
                SSD1306_DrawPixel(ssd1306.currentX + col, ssd1306.currentY + row,
                                  (color == SSD1306_COLOR_WHITE) ? SSD1306_COLOR_BLACK : SSD1306_COLOR_WHITE);
            }
            line >>= 1;
        }
    }

    // 1 column spacing
    for (uint8_t row = 0; row < 8; row++) {
        SSD1306_DrawPixel(ssd1306.currentX + 5, ssd1306.currentY + row,
                          (color == SSD1306_COLOR_WHITE) ? SSD1306_COLOR_BLACK : SSD1306_COLOR_WHITE);
    }

    ssd1306.currentX += 6;
    return true;
}

bool SSD1306_WriteString(const char* str, ssd1306_color_t color) {
    while (*str) {
        if (*str == '\n') {
            ssd1306.currentX = 0;
            ssd1306.currentY += 8;
            str++;
            continue;
        }
        if (!SSD1306_WriteChar(*str, color)) return false;
        str++;
    }
    return true;
}

// ---------- Drawing helpers ----------
void SSD1306_DrawLine(int x0, int y0, int x1, int y1, ssd1306_color_t color) {
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        SSD1306_DrawPixel((uint16_t)x0, (uint16_t)y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void SSD1306_DrawRectangle(int x, int y, int w, int h, ssd1306_color_t color) {
    SSD1306_DrawLine(x, y, x + w - 1, y, color);
    SSD1306_DrawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    SSD1306_DrawLine(x, y, x, y + h - 1, color);
    SSD1306_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void SSD1306_FillRectangle(int x, int y, int w, int h, ssd1306_color_t color) {
    for (int i = 0; i < h; i++) {
        SSD1306_DrawLine(x, y + i, x + w - 1, y + i, color);
    }
}

void SSD1306_DrawBitmap(int x, int y, const uint8_t* bitmap, int w, int h, ssd1306_color_t color) {
    // bitmap is 1bpp, row-major, MSB first per byte (common for icons)
    int byteWidth = (w + 7) / 8;

    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            uint8_t byte = bitmap[j * byteWidth + i / 8];
            bool pixelOn = (byte & (0x80 >> (i & 7))) != 0;
            if (pixelOn) {
                SSD1306_DrawPixel((uint16_t)(x + i), (uint16_t)(y + j), color);
            }
        }
    }
}

// ---------- Optional controls ----------
void SSD1306_SetContrast(uint8_t contrast) {
    ssd1306_write_command(0x81);
    ssd1306_write_command(contrast);
}

void SSD1306_SetDisplayOn(bool on) {
    ssd1306_write_command(on ? 0xAF : 0xAE);
}

void SSD1306_InvertDisplay(bool invert) {
    ssd1306.inverted = invert;
    ssd1306_write_command(invert ? 0xA7 : 0xA6);
}

void SSD1306_Debug_AllPixelsOn(void)
{
  ssd1306_write_command(0xA5); // Entire display ON
}

void SSD1306_Debug_ResumeRAM(void)
{
  ssd1306_write_command(0xA4); // Resume display from RAM
}

