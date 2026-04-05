#pragma once

#include "main.h"  // gives you I2C_HandleTypeDef (hi2c1 etc.)

// -------------------- USER CONFIG --------------------

// Put your I2C handle name here (CubeMX usually generates hi2c1 / hi2c2)
#define SSD1306_I2C_HANDLE      hi2c2

#define SSD1306_COL_OFFSET  2   // try 2 for SH1106, 0 for true SSD1306


// SSD1306 7-bit address is usually 0x3C or 0x3D.
// HAL expects left-shifted address (8-bit form).
#define SSD1306_I2C_ADDR        (0x3C << 1)

// Pick ONE display size:
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64   // change to 32 if you have 128x32

// I2C timeout
#define SSD1306_I2C_TIMEOUT_MS  100

// Chunk size for sending buffer over I2C (16..32 is safe)
#define SSD1306_I2C_CHUNK       16
