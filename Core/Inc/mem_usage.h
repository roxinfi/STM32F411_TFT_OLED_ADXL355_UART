#pragma once
#include <stdint.h>

uint32_t Mem_FlashUsed(void);
uint32_t Mem_FlashTotal(void);

uint32_t Mem_RamUsedStatic(void);   // .data + .bss
uint32_t Mem_RamTotal(void);
