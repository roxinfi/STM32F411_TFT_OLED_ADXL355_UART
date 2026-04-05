 #include "i2clcd.h"
#include <stdbool.h>
#include <string.h>

/* If you're using I2C2, switch these two lines:
extern I2C_HandleTypeDef hi2c1;
#define I2C_HANDLE (&hi2c1)
*/
extern I2C_HandleTypeDef hi2c2;
#define I2C_HANDLE (&hi2c2)

/* Most PCF8574 backpacks are 0x27 or 0x3F. Try the other if you see nothing. */
#define SLAVE_ADDRESS_LCD ((0x27) << 1)

#define LINES   4
#define COLUMNS 20

/* LCD DDRAM map */
static const uint8_t lcdPos[LINES][COLUMNS] = {
    {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13},
    {0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0x53},
    {0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27},
    {0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67}
};

/* Low-level senders (4-bit mode + enable strobes) */
/* lcd_send_cmd:=============================================================================
 * Author  : Vraj Patel
 * Date    : 2025-10-15
 * Modified: None
 * Desc : Send command to LCD via I2C to PCF8574
 * Inputs  : cmd - Command byte to send
 * Returns : None
 */
void lcd_send_cmd(char cmd)
{
    uint8_t data_u = (uint8_t)(cmd & 0xF0);
    uint8_t data_l = (uint8_t)((cmd << 4) & 0xF0);
    uint8_t data_t[4];
    data_t[0] = data_u | 0x0C; // en=1, rs=0
    data_t[1] = data_u | 0x08; // en=0, rs=0
    data_t[2] = data_l | 0x0C; // en=1, rs=0
    data_t[3] = data_l | 0x08; // en=0, rs=0
    HAL_I2C_Master_Transmit(I2C_HANDLE, SLAVE_ADDRESS_LCD, data_t, sizeof(data_t), 100);
}// eo lcd_send_cmd::

/* lcd_send_data:============================================================================
 * Author  : Vraj Patel
 * Date    : 2025-10-15
 * Modified: None
 * Desc : Send data to LCD via I2C to PCF8574
 * Inputs  : data - Data byte to send
 * Returns : None
 */
void lcd_send_data(char data)
{
    uint8_t data_u = (uint8_t)(data & 0xF0);
    uint8_t data_l = (uint8_t)((data << 4) & 0xF0);
    uint8_t data_t[4];
    data_t[0] = data_u | 0x0D; // en=1, rs=1
    data_t[1] = data_u | 0x09; // en=0, rs=1
    data_t[2] = data_l | 0x0D; // en=1, rs=1
    data_t[3] = data_l | 0x09; // en=0, rs=1
    HAL_I2C_Master_Transmit(I2C_HANDLE, SLAVE_ADDRESS_LCD, data_t, sizeof(data_t), 100);
}

/* Canonical clear (instruction 0x01) */

/* lcd_clear:===============================================================================
 * Author  : Vraj Patel
 * Date    : 2025-10-15
 * Modified: None
 * Desc : Clear the LCD display
 * Inputs  : None
 * Returns : None
 */
void lcd_clear(void)
{
    lcd_send_cmd(0x01);
    HAL_Delay(2); // datasheet ~1.53ms
}

/* lcd_init:===============================================================================
 * Author  : Vraj Patel
 * Date    : 2025-10-15
 * Modified: None
 * Desc : Initialize the LCD in 4-bit mode via I2C to PCF8574
 * Inputs  : None
 * Returns : None
 */
void lcd_init(void)
{
	// repeating 8-bit init sequence 3 times to ensure LCD is in 8-bit mode and then switching to 4-bit mode
    HAL_Delay(50);
    lcd_send_cmd(0x30); // 8-bit (might be 4-bit, but this is the init sequence)
    HAL_Delay(5);
    lcd_send_cmd(0x30); // 8-bit (might have bad timing)
    HAL_Delay(1);
    lcd_send_cmd(0x30);// 8-bit (might be in mid command)
    HAL_Delay(10);
    lcd_send_cmd(0x20);  // 4-bit
    HAL_Delay(10);

    lcd_send_cmd(0x28);  // 2 lines, 5x8 dots
    HAL_Delay(1);
    lcd_send_cmd(0x08);  // display off
    HAL_Delay(1);
    lcd_send_cmd(0x01);  // clear
    HAL_Delay(2);
    lcd_send_cmd(0x06);  // entry mode: increment, no shift
    HAL_Delay(1);
    lcd_send_cmd(0x0C);  // display on, cursor off, blink off
}// eo lcd_init::

/* UTF-8 handling for a few umlauts: maps "C3 A4/ B6/ BC/ 9F" to HD44780 ROM A approx.
   If you don’t need this, you can delete the if/else block and just send bytes. */

/* lcd_send_string:==========================================================================
 * Author  : Vraj Patel
 * Date    : 2025-10-15
 * Modified: None
 * Desc : Send a string to the LCD, handling some UTF-8 characters
 * Inputs  : str - Pointer to the null-terminated string
 * Returns : None
 */
void lcd_send_string(const char *str)
{
    while (*str)
    {
        unsigned char c = (unsigned char)*str;

        if (c == 0xC3)  // start of 'Ä/Ö/Ü/ä/ö/ü/ß' in UTF-8
        {
            str++;
            unsigned char d = (unsigned char)*str;

            // Map the second byte to HD44780 code page A approximations
            // ä (C3 A4) -> 0xE1, ö (C3 B6) -> 0xEF, ü (C3 BC) -> 0xF5, ß (C3 9F) -> 0xE2
            if (d == 0xA4) lcd_send_data((char)0xE1);       // ä
            else if (d == 0xB6) lcd_send_data((char)0xEF);  // ö
            else if (d == 0xBC) lcd_send_data((char)0xF5);  // ü
            else if (d == 0x9F) lcd_send_data((char)0xE2);  // ß
            else               lcd_send_data('?');          // fallback
            if (*str) str++;
            continue;
        }

        lcd_send_data((char)c);
        str++;
    }
}// eo lcd_send_string::

/* lcd_write:===============================================================================
 * Author  : Vraj Patel
 * Date    : 2025-10-15
 * Modified: None
 * Desc : Write a string to a specific position on the LCD
 * Inputs  : txt - Pointer to the null-terminated string
 *           line - Line number (0 to LINES-1)
 *           column - Column number (0 to COLUMNS-1)
 * Returns : None
 */
void lcd_write(const char *txt, uint8_t line, uint8_t column)
{
    if (line >= LINES || column >= COLUMNS) return;
    lcd_send_cmd(0x80 | lcdPos[line][column]);
    lcd_send_string(txt);
}// eo lcd_write::

/* lcd_clear_line:=========================================================================
 * Author  : Vraj Patel
 * Date    : 2025-10-15
 * Modified: None
 * Desc : Clear a specific line from a specific column to the end
 * Inputs  : line - Line number (0 to LINES-1)
 *           column - Column number (0 to COLUMNS-1)
 * Returns : None
 */
void lcd_clear_line(uint8_t line, uint8_t column)
{
    if (line >= LINES || column >= COLUMNS) return;
    lcd_send_cmd(0x80 | lcdPos[line][column]);
    for (int i = column; i < COLUMNS; i++)
        lcd_send_data(' ');
}// eo lcd_clear_line::
