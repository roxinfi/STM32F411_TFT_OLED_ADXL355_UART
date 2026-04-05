/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bme280.h"
#include "ssd1306.h"
#include "font5x7.h"
#include "ili9341.h"
#include "mem_usage.h"
#include "i2clcd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include "adxl335.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BME280_I2C_ADDR BME280_I2C_ADDR_0x76
#define SLEEP_DURATION_MS 2000
#define UART_BUFFER_SIZE 255
#define TFT_BUFFER_SIZE 320
#define ADC_MAX_VALUE 4095.0
#define VREF_MV 3300.0  // Reference voltage in millivolts
#define TRUE 1
#define FALSE 0
#define ON 1
#define OFF 0
#define HIGH 1
#define LOW 0
#define SENSORCOUNT 3 // Number of sensors
#define ADC_CH_COUNT 3 // Number of ADC channels used
// 20x4 LCD
#define ROWS 4
#define COLS 20
#define ADC_DMA_INDEX_IN0 0
#define ADC_DMA_INDEX_IN1 1
#define ADC_DMA_INDEX_IN2 2
#define HOURS_24 19
#define MINUTES_60 53

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */


volatile uint16_t adc_dma_buffer[ADC_CH_COUNT];
//static char lcd_line_1[COLS + 1];
//static char lcd_line_2[COLS + 1];
//static char lcd_line_3[COLS + 1];
//static char lcd_line_4[COLS + 1];
//char uartBuffer[UART_BUFFER_SIZE];
//char tftBuffer1[UART_BUFFER_SIZE];
//char tftBuffer2[UART_BUFFER_SIZE];
//char tftBuffer3[UART_BUFFER_SIZE];
//char tftBuffer4[UART_BUFFER_SIZE];
//char tftBuffer5[UART_BUFFER_SIZE];
//char tftBuffer6[UART_BUFFER_SIZE];
//char tftBuffer7[UART_BUFFER_SIZE];
char dateStr[16];
char timeStr[16];
volatile uint8_t tick100ms = 0;
volatile uint8_t tick500ms = 0;
volatile uint8_t tick1s = 0;
volatile uint32_t err_code = 0;
static uint32_t arr5=0, arr4=0, arr10=0;


#define CHECK_OK(expr, code) do { \
  err_code = (code); \
  if ((expr) != HAL_OK) Error_Handler(); \
} while(0)


typedef struct
{
	char uartBuffer[UART_BUFFER_SIZE];
} usart_data_t;

typedef struct
{
	char tftBuffer[TFT_BUFFER_SIZE];
} tft_data_t;

typedef struct
{
	char lcdBuffer[COLS + 1];
} lcd_data_t;

lcd_data_t lcd_data[ROWS]; // For 4 LCD lines

tft_data_t tft_data[10]; // For 10 TFT messages

usart_data_t usart_data[2]; // For USART1 and USART6

BME280_Handle bme280;

ADXL335_State adxl;


/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c2;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim10;
TIM_HandleTypeDef htim11;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */

extern SPI_HandleTypeDef hspi1;

extern uint8_t _end;

ili9341_t tft = {
	.hspi = &hspi1,
	.cs_port = TFT_CS_GPIO_Port,
	.cs_pin  = TFT_CS_Pin,
	.dc_port = TFT_DC_GPIO_Port,
	.dc_pin  = TFT_DC_Pin,
	.rst_port= TFT_RESET_GPIO_Port,
	.rst_pin = TFT_RESET_Pin
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_RTC_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_TIM5_Init(void);
static void MX_TIM11_Init(void);
static void MX_TIM4_Init(void);
static void MX_TIM10_Init(void);
/* USER CODE BEGIN PFP */
static void uart_1_print(const char *s);
static void uart_6_print(const char *s);
static void display_time_date(void);
static void RTC_GetDateTimeStrings(char *dateOut, size_t dateSz, char *timeOut, size_t timeSz);
//static uint16_t ADC1_ReadChannel(uint32_t channel);
static void int_lcd_buffer(lcd_data_t *lcd);
static void int_tft_buffer(tft_data_t *tft);
static void init_uart_buffer(usart_data_t *usart);
static inline uint32_t get_msp(void);
uint32_t Ram_FreeNow(void);
static uint16_t checkSum(const char *str);
static void I2C2_BusRecover(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  uint32_t flash_used = Mem_FlashUsed();
  uint32_t flash_total = Mem_FlashTotal();
  uint32_t sram_used = Mem_RamUsedStatic();
  uint32_t sram_total = Mem_RamTotal();
  static float bme280_Temp = 0.0;
  static float bme280_Pres = 0.0;
  static float bme280_Hum = 0.0;
  static volatile uint32_t live_free_ram = 0;
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  I2C2_BusRecover();
  MX_I2C2_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_RTC_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_USART6_UART_Init();
  MX_TIM5_Init();
  MX_TIM11_Init();
  MX_TIM4_Init();
  MX_TIM10_Init();
  /* USER CODE BEGIN 2 */
  // BME280_Status bme280_init(BME280_Handle *dev, I2C_HandleTypeDef *hi2c, uint8_t i2c_addr)
  //intialize clear of the buffer for lcd, tft and uart
  for(uint8_t i = 0; i < ROWS; i++)
  {
	  int_lcd_buffer(&lcd_data[i]);
  }
  for(uint8_t i = 0; i < 10; i++)
  {
	  int_tft_buffer(&tft_data[i]);
  }
  for(uint8_t i = 0; i < 2; i++)
  {
	  init_uart_buffer(&usart_data[i]);
  }
  SSD1306_Init();
  // Force all pixels ON (if comms + init OK)
  SSD1306_InvertDisplay(false);
  HAL_Delay(50);

  // Command 0xA5 = Entire display ON (ignores RAM)
  extern void SSD1306_Debug_AllPixelsOn(void);
  SSD1306_Debug_AllPixelsOn();
  HAL_Delay(1000);
  SSD1306_Debug_ResumeRAM();

  SSD1306_Fill(SSD1306_COLOR_BLACK);
  SSD1306_SetCursor(0, 0);
  SSD1306_WriteString("Hello World", SSD1306_COLOR_WHITE);
  SSD1306_SetCursor(0, 16);
  SSD1306_WriteString("BME280 Test", SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();
  bme280_init(&bme280, &hi2c2, BME280_I2C_ADDR);
  lcd_init();
  HAL_Delay(50);
  lcd_clear();
  HAL_Delay(50);
  ILI9341_Init(&tft);
  HAL_Delay(50);
  ILI9341_SetRotation(&tft, 1); // Landscape mode
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET); // CS high
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); // DC high
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET); // RESET high

  ILI9341_FillScreen(&tft, ILI9341_BLACK);
  ILI9341_DrawString(&tft, 10, 0, "TFT OK", ILI9341_YELLOW, ILI9341_BLACK, 1);

  HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_4);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim10, TIM_CHANNEL_1);

  arr5  = __HAL_TIM_GET_AUTORELOAD(&htim5);
  arr4  = __HAL_TIM_GET_AUTORELOAD(&htim4);
  arr10 = __HAL_TIM_GET_AUTORELOAD(&htim10);

  for (uint16_t d = 0; d < arr4; d += (arr4/50)) {
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, d);
    HAL_Delay(20);
  }
  for (int d = arr4; d >= 0; d -= (arr4/50)) {
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, d);
    HAL_Delay(20);
  }

  for (uint16_t d = 0; d < arr5; d += (arr5/50)) {
	__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_4, d);
	HAL_Delay(20);
  }

  for (int d = arr5; d >= 0; d -= (arr5/50)) {
	  __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_4, d);
	  HAL_Delay(20);
  }

  for (uint16_t d = 0; d < arr10; d += (arr10/50)) {
	  __HAL_TIM_SET_COMPARE(&htim10, TIM_CHANNEL_1, d);
	  HAL_Delay(20);
  }

  for (int d = arr10; d >= 0; d -= (arr10/50)) {
	  __HAL_TIM_SET_COMPARE(&htim10, TIM_CHANNEL_1, d);
	  HAL_Delay(20);
  }

  HAL_TIM_Base_Start_IT(&htim11); // your 100ms tick

  // start ADC DMA ONLY ONCE
  CHECK_OK(HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_dma_buffer, ADC_CH_COUNT), 1);

  ADXL335_InitDefaults(&adxl);
  HAL_Delay(200);
  ADXL335_CalibrateFlat(&adxl,800);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  display_time_date();
	  if(tick100ms)
	  {
		  tick100ms = 0;
		  tick1s++;
		  live_free_ram = Ram_FreeNow();
		  ADXL335_Update(&adxl);

		  uint16_t ccr5  = ADXL335_TiltToPwmCcr(&adxl, arr5);
		  uint16_t ccr4  = ADXL335_TiltToPwmCcr(&adxl, arr4);
		  uint16_t ccr10 = ADXL335_TiltToPwmCcr(&adxl, arr10);

		  __HAL_TIM_SET_COMPARE(&htim5,  TIM_CHANNEL_4, ccr5);
		  __HAL_TIM_SET_COMPARE(&htim4,  TIM_CHANNEL_1, ccr4);
		  __HAL_TIM_SET_COMPARE(&htim10, TIM_CHANNEL_1, ccr10);

		  if(HAL_GPIO_ReadPin(SW_1_GPIO_Port, SW_1_Pin) == GPIO_PIN_RESET)
		  {
			  HAL_GPIO_WritePin(GREEN_1_LED_GPIO_Port, GREEN_1_LED_Pin, GPIO_PIN_SET);
			  MX_I2C2_Init();
		  	  HAL_Delay(100);
			  MX_SPI1_Init();
			  HAL_Delay(100);
		 	  lcd_clear();
 			  HAL_Delay(50);
 			  lcd_init();
 			  HAL_Delay(50);
		  	  lcd_clear();
		  	  HAL_GPIO_WritePin(GREEN_1_LED_GPIO_Port, GREEN_1_LED_Pin, GPIO_PIN_RESET);
		  }


		  // select a random sensor channel between 0 and SENSORCOUNT-1
		  uint8_t idx = (rand() % ADC_CH_COUNT);   // idx = 0..ADC_CH_COUNT-1
		  uint16_t adcValue = adc_dma_buffer[idx];
		  float voltage_mV = ((float)adcValue * (float)VREF_MV) / (float)ADC_MAX_VALUE;

		  bme280_read_all(&bme280, &bme280_Temp, &bme280_Pres, &bme280_Hum);

		  tick500ms++;
		  if(tick500ms >= 5)
		  {
			  tick500ms = 0;
			  lcd_clear();
			  snprintf(lcd_data[0].lcdBuffer, COLS + 1, "T:%.1f%cC H:%.1f%%", bme280_Temp, (char)0xDF, bme280_Hum);
			  snprintf(lcd_data[1].lcdBuffer, COLS + 1, "P:%.1fhPa", bme280_Pres / 100.0);
			  snprintf(lcd_data[2].lcdBuffer, COLS + 1, "ADC%d:%4u->V:%4umV", (unsigned)(idx+1), (unsigned)adcValue, (uint16_t)voltage_mV);
			  snprintf(lcd_data[3].lcdBuffer, COLS + 1, "Sleep:%ums", SLEEP_DURATION_MS);
			  lcd_write(lcd_data[0].lcdBuffer, 0, 0);
			  lcd_write(lcd_data[1].lcdBuffer, 1, 0);
			  lcd_write(lcd_data[2].lcdBuffer, 2, 0);
			  lcd_write(lcd_data[3].lcdBuffer, 3, 0);
		  }

		  HAL_GPIO_TogglePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin);


		  if(bme280_Temp < 18.0 || bme280_Temp > 26.0)
		  {
			  HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
		  }
		  else
		  {
			  HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
		  }

		  if(bme280_Hum < 10.0 || bme280_Hum > 50.0)
		  {
			  HAL_GPIO_WritePin(YELLOW_LED_GPIO_Port, YELLOW_LED_Pin, GPIO_PIN_SET);
		  }
		  else
		  {
			  HAL_GPIO_WritePin(YELLOW_LED_GPIO_Port, YELLOW_LED_Pin, GPIO_PIN_RESET);
		  }

		  if(tick1s >= 10)
		  {
			  tick1s = 0;
			  // clear terminal
			  snprintf(usart_data[1].uartBuffer, UART_BUFFER_SIZE, "\033[2J\033[H");
			  uart_1_print(usart_data[1].uartBuffer);

			  // Print ADC reading
			  snprintf(usart_data[1].uartBuffer, UART_BUFFER_SIZE, "ADC Rank: %u, ADC Value: %u, Voltage: %.2f mV\r\n", (unsigned)(idx+1), (unsigned)adcValue, voltage_mV);
			  uart_1_print(usart_data[1].uartBuffer);

			  // Read BME280 sensor data
			  snprintf(usart_data[1].uartBuffer, UART_BUFFER_SIZE, "BME280 - Temp: %.1f°C, Humidity: %.1f %%, Pressure: %.1f hPa\r\n", bme280_Temp, bme280_Hum, bme280_Pres / 100.0);
			  uart_1_print(usart_data[1].uartBuffer);

			  // Print memory usage
			  snprintf(usart_data[1].uartBuffer, UART_BUFFER_SIZE,"FLASH Used: %lu / %lu bytes => %u%%/100%%\r\n", (unsigned long)flash_used, (unsigned long)flash_total, (unsigned)((flash_used * 100UL) / flash_total));
			  uart_1_print(usart_data[1].uartBuffer);

			  // Print SRAM usage
			  snprintf(usart_data[1].uartBuffer, UART_BUFFER_SIZE,"SRAM Used: %lu / %lu bytes => %u%%/100%%\r\n", (unsigned long)sram_used, (unsigned long)sram_total, (unsigned)((sram_used * 100UL) / sram_total));
			  uart_1_print(usart_data[1].uartBuffer);

			  // Print live free RAM
			  snprintf(usart_data[1].uartBuffer, UART_BUFFER_SIZE,"Live Free RAM: %lu bytes\r\n", (unsigned long)live_free_ram);
			  uart_1_print(usart_data[1].uartBuffer);

			  int n = snprintf(usart_data[0].uartBuffer, UART_BUFFER_SIZE,
			      "$%u,%u,%.2f,%.1f,%.1f,%.1f,%lu,%lu,%u,%lu,%lu,%u,%lu",
			      (unsigned)(idx+1),(unsigned)adcValue,voltage_mV,
			      bme280_Temp,bme280_Hum,bme280_Pres / 100.0,
			      (unsigned long)flash_used,(unsigned long)flash_total,
			      (unsigned)((flash_used * 100UL) / flash_total),
			      (unsigned long)sram_used,(unsigned long)sram_total,
			      (unsigned)((sram_used * 100UL) / sram_total),
			      (unsigned long)live_free_ram
			  );

			  if (n > 0 && n < (UART_BUFFER_SIZE - 12))
			  {
			      uint16_t cs = checkSum(usart_data[0].uartBuffer);

			      // IMPORTANT: add comma + checksum + \r\n
			      snprintf(usart_data[0].uartBuffer + n, UART_BUFFER_SIZE - n,
			               ",%u\r\n", (unsigned)cs);

			      uart_1_print(usart_data[0].uartBuffer);
			  }

		  }


		  // Prepare TFT display strings
		  snprintf(tft_data[0].tftBuffer, UART_BUFFER_SIZE, "Temp: %.1f%cC", bme280_Temp, (char)0xDF);
		  snprintf(tft_data[1].tftBuffer, UART_BUFFER_SIZE, "Hum:  %.1f%%", bme280_Hum);
		  snprintf(tft_data[2].tftBuffer, UART_BUFFER_SIZE, "Pres: %.1fhPa", bme280_Pres / 100.0);
		  snprintf(tft_data[3].tftBuffer, UART_BUFFER_SIZE, "Live RAM: %lu Bytes", (unsigned long)live_free_ram);

		  ILI9341_FillRect(&tft, 0, 240, tft.width, 100, ILI9341_BLACK);
		  // Draw the text
		  ILI9341_DrawString(&tft, 10, 8, tft_data[0].tftBuffer, ILI9341_CYAN, ILI9341_BLACK, 1);
		  ILI9341_DrawString(&tft, 10, 16, tft_data[1].tftBuffer, ILI9341_CYAN, ILI9341_BLACK, 1);
		  ILI9341_DrawString(&tft, 10, 24, tft_data[2].tftBuffer, ILI9341_CYAN, ILI9341_BLACK, 1);
		  ILI9341_DrawString(&tft, 10, 32, tft_data[3].tftBuffer, ILI9341_CYAN, ILI9341_BLACK, 1);

	  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 3;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = 3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0;
  sTime.Minutes = 0;
  sTime.Seconds = 0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 19;
  sDate.Year = 0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the WakeUp
  */
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 49;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 47;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 999;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 47;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 999;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */
  HAL_TIM_MspPostInit(&htim5);

}

/**
  * @brief TIM10 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM10_Init(void)
{

  /* USER CODE BEGIN TIM10_Init 0 */

  /* USER CODE END TIM10_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM10_Init 1 */

  /* USER CODE END TIM10_Init 1 */
  htim10.Instance = TIM10;
  htim10.Init.Prescaler = 47;
  htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim10.Init.Period = 999;
  htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim10, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM10_Init 2 */

  /* USER CODE END TIM10_Init 2 */
  HAL_TIM_MspPostInit(&htim10);

}

/**
  * @brief TIM11 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM11_Init(void)
{

  /* USER CODE BEGIN TIM11_Init 0 */

  /* USER CODE END TIM11_Init 0 */

  /* USER CODE BEGIN TIM11_Init 1 */

  /* USER CODE END TIM11_Init 1 */
  htim11.Instance = TIM11;
  htim11.Init.Prescaler = 4799;
  htim11.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim11.Init.Period = 999;
  htim11.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim11.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim11) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM11_Init 2 */

  /* USER CODE END TIM11_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_DisableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, TFT_DC_Pin|TFT_RESET_Pin|TFT_CS_Pin|GREEN_1_LED_Pin
                          |YELLOW_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BLUE_LED_Pin */
  GPIO_InitStruct.Pin = BLUE_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BLUE_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SW_1_Pin */
  GPIO_InitStruct.Pin = SW_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SW_1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TFT_DC_Pin TFT_RESET_Pin TFT_CS_Pin GREEN_1_LED_Pin
                           YELLOW_LED_Pin */
  GPIO_InitStruct.Pin = TFT_DC_Pin|TFT_RESET_Pin|TFT_CS_Pin|GREEN_1_LED_Pin
                          |YELLOW_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : SW_2_Pin SW_3_Pin */
  GPIO_InitStruct.Pin = SW_2_Pin|SW_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : RED_LED_Pin */
  GPIO_InitStruct.Pin = RED_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RED_LED_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* static uint16_t ADC1_ReadChannel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel      = channel;                     // e.g. ADC_CHANNEL_0
    sConfig.Rank         = 1;                           // single conversion
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;     // safe default
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) return 0;

    if (HAL_ADC_Start(&hadc1) != HAL_OK) return 0;

    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) // 10 ms timeout
    {
        HAL_ADC_Stop(&hadc1);
        return 0;
    }

    uint16_t value = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return value;   // 0..4095 for 12-bit
}// eo ADC1_ReadChannel::
*/

static void uart_1_print(const char *s)
{
  HAL_UART_Transmit(&huart1, (uint8_t*)s, (uint16_t)strlen(s), 100);
}// eo uart_1_print::

static void uart_6_print(const char *s)
{
  HAL_UART_Transmit(&huart6, (uint8_t*)s, (uint16_t)strlen(s), 100);
}// eo uart_6_print::


static void display_time_date(void)
{
	  RTC_GetDateTimeStrings(dateStr, sizeof(dateStr), timeStr, sizeof(timeStr));
	  SSD1306_SetCursor(0, 32);
	  SSD1306_WriteString(dateStr, SSD1306_COLOR_WHITE);
	  SSD1306_SetCursor(0, 48);
	  SSD1306_WriteString(timeStr, SSD1306_COLOR_WHITE);
	  SSD1306_UpdateScreen();
	  //snprintf(tftBuffer5, UART_BUFFER_SIZE, dateStr);
	  snprintf(tft_data[4].tftBuffer, UART_BUFFER_SIZE, dateStr);
	  //snprintf(tftBuffer6, UART_BUFFER_SIZE, timeStr);
	  snprintf(tft_data[5].tftBuffer, UART_BUFFER_SIZE, timeStr);
	  //ILI9341_DrawString(&tft, 10, 50, tftBuffer5, ILI9341_CYAN, ILI9341_BLACK, 2);
	  ILI9341_DrawString(&tft, 10, 50, tft_data[4].tftBuffer, ILI9341_CYAN, ILI9341_BLACK, 2);
	  //ILI9341_DrawString(&tft, 10, 90, tftBuffer6, ILI9341_CYAN, ILI9341_BLACK, 2);
	  ILI9341_DrawString(&tft, 10, 90, tft_data[5].tftBuffer, ILI9341_CYAN, ILI9341_BLACK, 2);
}


static void init_uart_buffer(usart_data_t *uart)
{
	// clear structure
	for(uint16_t i = 0; i < UART_BUFFER_SIZE; i++)
	{
		uart->uartBuffer[i] = 0;
	}
}

static void int_tft_buffer(tft_data_t *tft)
{
	// clear structure
	for(uint16_t i = 0; i < TFT_BUFFER_SIZE; i++)
	{
		tft->tftBuffer[i] = 0;
	}
}

static void int_lcd_buffer(lcd_data_t *lcd)
{
	// clear structure
	for(uint16_t i = 0; i < (COLS + 1); i++)
	{
		lcd->lcdBuffer[i] = 0;
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM11)
	{
		tick100ms++;
	}
}

static inline uint32_t get_msp(void)
{
	uint32_t sp;
	__asm volatile ("MRS %0, MSP" : "=r" (sp) );
	return sp;
}

uint32_t Ram_FreeNow(void)
{
	uint8_t *heap_end = (uint8_t*)&_end;
	uint32_t sp = get_msp();
	if((uint32_t)heap_end >= sp)return 0;
	return (uint32_t)(sp - (uint32_t)heap_end);
}

static void RTC_GetDateTimeStrings(char *dateOut, size_t dateSz,
                            char *timeOut, size_t timeSz)
{
    RTC_TimeTypeDef t;
    RTC_DateTypeDef d;

    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);

    uint16_t yyyy = 2000 + d.Year;

    snprintf(dateOut, dateSz, "%02u:%02u:%04u", d.Date, d.Month, yyyy);
    snprintf(timeOut, timeSz, "%02u:%02u:%02u", t.Hours, t.Minutes, t.Seconds);
}

static uint16_t checkSum(const char *msg)
{
    uint16_t sum = 0;
    while (*msg) {
        sum += (uint8_t)(*msg++);
    }
    return sum;
}

static void I2C2_BusRecover(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Disable I2C2 peripheral
    __HAL_RCC_I2C2_CLK_DISABLE();

    // Configure SCL/SDA as open-drain outputs with pull-ups
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    GPIO_InitStruct.Pin = GPIO_PIN_10;              // PB10 = I2C2_SCL (change if different)
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11;              // PB11 = I2C2_SDA (change if different)
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Release lines
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
    HAL_Delay(1);

    // Clock SCL up to 9 pulses if SDA is stuck low
    for (int i = 0; i < 9; i++)
    {
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_SET)
            break;

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
        HAL_Delay(1);
    }

    // Generate STOP: SDA low -> SCL high -> SDA high
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
    HAL_Delay(1);

    // Re-enable I2C2 clock (MX_I2C2_Init will restore AF mode)
    __HAL_RCC_I2C2_CLK_ENABLE();
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
		HAL_GPIO_TogglePin(RED_LED_GPIO_Port, RED_LED_Pin);
		HAL_Delay(100);
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
