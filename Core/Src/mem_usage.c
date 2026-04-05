#include "mem_usage.h"
#include <stdint.h>

#define FLASH_TOTAL_BYTES (512U * 1024U)
#define SRAM_TOTAL_BYTES  (128U * 1024U)

/* Linker symbols that already exist in CubeIDE scripts */
extern uint8_t _sdata, _edata, _sidata;
extern uint8_t _sbss,  _ebss;
extern uint8_t _etext;

uint32_t Mem_FlashTotal(void)
{
    return FLASH_TOTAL_BYTES;
}

/* Estimate flash used = end of flash image - flash origin.
   For CubeIDE FLASH script, flash origin is 0x08000000.
*/
uint32_t Mem_FlashUsed(void)
{
    uint32_t data_size = (uint32_t)(&_edata - &_sdata);

    /* End of flash image: init values for .data in flash start at _sidata */
    uint32_t image_end = (uint32_t)(uintptr_t)&_sidata + data_size;

    /* Ensure at least past end of text */
    uint32_t etext = (uint32_t)(uintptr_t)&_etext;
    if (image_end < etext) image_end = etext;

    return (image_end - 0x08000000UL);
}

uint32_t Mem_RamTotal(void)
{
    return SRAM_TOTAL_BYTES;
}

uint32_t Mem_RamUsedStatic(void)
{
    uint32_t data_size = (uint32_t)(&_edata - &_sdata);
    uint32_t bss_size  = (uint32_t)(&_ebss  - &_sbss);
    return data_size + bss_size;
}
