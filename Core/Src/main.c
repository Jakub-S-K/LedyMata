/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
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
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
// #include "TempLeds.h"
#include "dupaleds.h"
#include <memory.h>
#include <stdio.h>
#include "usbd_customhid.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
extern USBD_HandleTypeDef hUsbDeviceFS;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC_CHANNEL_COUNT 4
#define ADC_DMA_BUFFER_SIZE (ADC_CHANNEL_COUNT)
#define SECTION_CHANNEL_UP 1
#define SECTION_CHANNEL_DOWN 0
#define SECTION_CHANNEL_LEFT 0
#define SECTION_CHANNEL_RIGHT 1
#define SECTION_CHANNEL_CENTER 1

#define SECTION_INDEX_UP 0
#define SECTION_INDEX_DOWN 1
#define SECTION_INDEX_LEFT 0
#define SECTION_INDEX_RIGHT 1
#define SECTION_INDEX_CENTER 2

#define ADC_INDEX_UP 0
#define ADC_INDEX_DOWN 2
#define ADC_INDEX_LEFT 3
#define ADC_INDEX_RIGHT 1

#define ADC_HYSTERESIS 20
#define ADC_MINIMAL_UP 2550
#define ADC_MINIMAL_DOWN 1800
#define ADC_MINIMAL_LEFT 1550
#define ADC_MINIMAL_RIGHT 2200
// #define ADC_MINIMAL_UP 4000
// #define ADC_MINIMAL_DOWN 4000
// #define ADC_MINIMAL_LEFT 2600
// #define ADC_MINIMAL_RIGHT 4000

static_assert(ADC_INDEX_UP != ADC_INDEX_DOWN);
static_assert(ADC_INDEX_DOWN != ADC_INDEX_LEFT);
static_assert(ADC_INDEX_LEFT != ADC_INDEX_RIGHT);
static_assert(ADC_INDEX_RIGHT != ADC_INDEX_UP);

static_assert(ADC_INDEX_UP < ADC_CHANNEL_COUNT);
static_assert(ADC_INDEX_DOWN < ADC_CHANNEL_COUNT);
static_assert(ADC_INDEX_LEFT < ADC_CHANNEL_COUNT);
static_assert(ADC_INDEX_RIGHT < ADC_CHANNEL_COUNT);
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#define GET_INDEX_FROM_CHANNEL(CHANNEL) (CHANNEL / (TIM_CHANNEL_2 - TIM_CHANNEL_1))

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch1;
DMA_HandleTypeDef hdma_tim1_ch2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
uint16_t gAdcBuffer[ADC_DMA_BUFFER_SIZE];
uint16_t gAdcMinimalValues[ADC_CHANNEL_COUNT];
uint8_t gAdcToSectionMapping[ADC_CHANNEL_COUNT];
uint8_t gAdcToChannelMapping[ADC_CHANNEL_COUNT];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint16_t gLedCh1Buffer[24 * 8];
uint8_t gLedCh1BufferIndex = 0;
uint8_t gLedCh1SectionIndex = 0;
uint8_t gLedCh1LedIndex = 0;
uint8_t gLedCh1ZeroIndex = 0;

uint16_t gLedCh2Buffer[24 * 8];
uint8_t gLedCh2BufferIndex = 0;
uint8_t gLedCh2SectionIndex = 0;
uint8_t gLedCh2LedIndex = 0;
uint8_t gLedCh2ZeroIndex = 0;

uint8_t gInterruptCounter = 0;
uint8_t gInterruptCounterHalf = 0;
uint8_t gInterruptCounterFull = 0;

struct
{
  union
  {
    uint8_t value;
    struct
    {
      uint8_t first : 1;
      uint8_t second : 1;
      uint8_t third : 1;
      uint8_t fourth : 1;
    };
  };
} gButtonState;

uint8_t StopDmaCh1 = 0;
uint8_t StopDmaCh2 = 0;
uint8_t ZeroDmaCh1 = 0;
uint8_t ZeroDmaCh2 = 0;

void interrupt(TIM_HandleTypeDef *htim)
{
  if (htim != &htim1)
  {
    return;
  }

  if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
  {
    if (StopDmaCh1)
    {
      HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1);
      StopDmaCh1 = 0;
      ZeroDmaCh1 = 0;
    }
    else if (ZeroDmaCh1)
    {
      StopDmaCh1 = FillHalfBufferWithZeros(0, gLedCh1Buffer);
    }
    else
    {
      ZeroDmaCh1 = FillHalfBuffer(0, gLedCh1Buffer);
      // uint8_t message[] = "dc1;";
      // HAL_UART_Transmit(&huart1, message, sizeof(message), 100);
    }
  }
  else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
  {
    if (StopDmaCh2)
    {
      HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_2);
      StopDmaCh2 = 0;
      ZeroDmaCh2 = 0;
    }
    else if (ZeroDmaCh2)
    {
      StopDmaCh2 = FillHalfBufferWithZeros(1, gLedCh2Buffer);
    }
    else
    {
      ZeroDmaCh2 = FillHalfBuffer(1, gLedCh2Buffer);
      // uint8_t message[] = "dc2;";
      // HAL_UART_Transmit(&huart1, message, sizeof(message), 100);
    }
  }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
  if (htim != &htim1)
  {
    return;
  }

  if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
  {
    if (StopDmaCh1)
    {
      HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1);
      StopDmaCh1 = 0;
      ZeroDmaCh1 = 0;
    }
    else if (ZeroDmaCh1)
    {
      StopDmaCh1 = FillHalfBufferWithZeros(0, gLedCh1Buffer);
    }
    else
    {
      ZeroDmaCh1 = FillHalfBuffer(0, gLedCh1Buffer);
      // uint8_t message[] = "dc1;";
      // HAL_UART_Transmit(&huart1, message, sizeof(message), 100);
    }
  }
  else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
  {
    if (StopDmaCh2)
    {
      HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_2);
      StopDmaCh2 = 0;
      ZeroDmaCh2 = 0;
    }
    else if (ZeroDmaCh2)
    {
      StopDmaCh2 = FillHalfBufferWithZeros(1, gLedCh2Buffer);
    }
    else
    {
      ZeroDmaCh2 = FillHalfBuffer(1, gLedCh2Buffer);
      // uint8_t message[] = "dc2;";
      // HAL_UART_Transmit(&huart1, message, sizeof(message), 100);
    }
  }
  // interrupt(htim);
}

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim)
{
  if (htim != &htim1)
  {
    return;
  }

  if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
  {
    if (StopDmaCh1)
    {
      HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1);
      StopDmaCh1 = 0;
      ZeroDmaCh1 = 0;
    }
    else if (ZeroDmaCh1)
    {
      StopDmaCh1 = FillHalfBufferWithZeros(0, gLedCh1Buffer);
    }
    else
    {
      ZeroDmaCh1 = FillHalfBuffer(0, gLedCh1Buffer);
      // uint8_t message[] = "dc1;";
      // HAL_UART_Transmit(&huart1, message, sizeof(message), 100);
    }
  }
  else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
  {
    if (StopDmaCh2)
    {
      HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_2);
      StopDmaCh2 = 0;
      ZeroDmaCh2 = 0;
    }
    else if (ZeroDmaCh2)
    {
      StopDmaCh2 = FillHalfBufferWithZeros(1, gLedCh2Buffer);
    }
    else
    {
      ZeroDmaCh2 = FillHalfBuffer(1, gLedCh2Buffer);
      // uint8_t message[] = "dc2;";
      // HAL_UART_Transmit(&huart1, message, sizeof(message), 100);
    }
  }
  // interrupt(htim);
}

#define ADC_MINIMUM_VALUE 800

uint32_t gAdcLastClick[ADC_CHANNEL_COUNT];
#define DEBOUNCE_TIME 40

void AdcInitializeData(void)
{
  memset(&gAdcBuffer, 0, sizeof(gAdcBuffer));
  memset(&gAdcLastClick, 0, sizeof(gAdcLastClick));
  gAdcMinimalValues[ADC_INDEX_UP] = ADC_MINIMAL_UP;
  gAdcMinimalValues[ADC_INDEX_DOWN] = ADC_MINIMAL_DOWN;
  gAdcMinimalValues[ADC_INDEX_LEFT] = ADC_MINIMAL_LEFT;
  gAdcMinimalValues[ADC_INDEX_RIGHT] = ADC_MINIMAL_RIGHT;
  gAdcToSectionMapping[ADC_INDEX_UP] = SECTION_INDEX_UP;
  gAdcToSectionMapping[ADC_INDEX_DOWN] = SECTION_INDEX_DOWN;
  gAdcToSectionMapping[ADC_INDEX_LEFT] = SECTION_INDEX_LEFT;
  gAdcToSectionMapping[ADC_INDEX_RIGHT] = SECTION_INDEX_RIGHT;
  gAdcToChannelMapping[ADC_INDEX_UP] = SECTION_CHANNEL_UP;
  gAdcToChannelMapping[ADC_INDEX_DOWN] = SECTION_CHANNEL_DOWN;
  gAdcToChannelMapping[ADC_INDEX_LEFT] = SECTION_CHANNEL_LEFT;
  gAdcToChannelMapping[ADC_INDEX_RIGHT] = SECTION_CHANNEL_RIGHT;
}

uint8_t GetButtonState(uint8_t Channel)
{
  if (gButtonState.value & (1 << Channel))
  {
    if (HAL_GetTick() - gAdcLastClick[Channel] > DEBOUNCE_TIME)
    {
      if (gAdcBuffer[Channel] <= (gAdcMinimalValues[Channel] - ADC_HYSTERESIS))
      {
        gButtonState.value = gButtonState.value ^ (1 << Channel);
        return 0;
      }
    }
  }
  else
  {
    if (gAdcBuffer[Channel] >= gAdcMinimalValues[Channel])
    {
      gAdcLastClick[Channel] = HAL_GetTick();
      gButtonState.value = gButtonState.value | (1 << Channel);
      return 1;
    }
  }
  return 0xFF;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  uint8_t AdcChannel;
  uint8_t DmaChannel;
  uint8_t Section;
  uint8_t ButtonState;
  uint8_t UpdateUsb;

  UpdateUsb = 0;
  for (AdcChannel = 0; AdcChannel < ADC_CHANNEL_COUNT; AdcChannel++)
  {
    Section = gAdcToSectionMapping[AdcChannel];
    DmaChannel = gAdcToChannelMapping[AdcChannel];
    ButtonState = GetButtonState(AdcChannel);
    if (ButtonState == 1)
    {
      GetLedSection(DmaChannel, Section)->Color = (COLOR){63, 236, 24};
      UpdateUsb = 1;
    }
    else if (ButtonState == 0)
    {
      GetLedSection(DmaChannel, Section)->Color = (COLOR){139, 0, 155};
      UpdateUsb = 1;
    }
  }
  if (UpdateUsb)
  {
    //  uint8_t message[] = "USB update;";
    //  HAL_UART_Transmit(&huart1, message, sizeof(message), 100);
    USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, (uint8_t *)&gButtonState, sizeof(gButtonState));
    if (hdma_tim1_ch1.State == HAL_DMA_STATE_READY)
    {
      if (PrepareBufferForTransaction(0, gLedCh1Buffer) != 0)
      {
        HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)gLedCh1Buffer, 24 * 8);
      }
      //  if (InitializeMemoryForDmaTransaction(gLedCh1Buffer, 8, 0, &gLedCh1BufferIndex, &gLedCh1LedIndex, &gLedCh1SectionIndex) != 0) {
      //    // uint8_t message[] = "d1;";
      //    // HAL_UART_Transmit(&huart1, message, sizeof(message), 100);
      //    HAL_TIM_PWM_Start_DMA (&htim1, TIM_CHANNEL_1, (uint32_t*)gLedCh1Buffer, 24 * 8);
      //  }
    }
    if (hdma_tim1_ch2.State == HAL_DMA_STATE_READY)
    {
      if (PrepareBufferForTransaction(1, gLedCh2Buffer) != 0)
      {
        HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (uint32_t *)gLedCh2Buffer, 24 * 8);
      }
      //  if (InitializeMemoryForDmaTransaction(gLedCh2Buffer, 8, 1, &gLedCh2BufferIndex, &gLedCh2LedIndex, &gLedCh2SectionIndex) != 0) {
      //      // uint8_t message[] = "d2;";
      //      // HAL_UART_Transmit(&huart1, message, sizeof(message), 100);
      //    HAL_TIM_PWM_Start_DMA (&htim1, TIM_CHANNEL_2, (uint32_t*)gLedCh2Buffer, 24 * 8);
      //  }
    }
  }
  //  uint8_t AdcChannel;
  //  uint8_t DmaChannel;
  //  uint8_t Section;
  //  uint8_t ButtonState;
  //  uint8_t UpdateUsb;
  //
  //  if (hadc != &hadc1) {
  //    return;
  //  }
  //
  //  UpdateUsb = 0;
  //  for (AdcChannel = 0; AdcChannel < ADC_CHANNEL_COUNT; AdcChannel++) {
  //    Section = gAdcToSectionMapping[AdcChannel];
  //    DmaChannel = gAdcToChannelMapping[AdcChannel];
  //    ButtonState = GetButtonState (AdcChannel);
  //    if (ButtonState == 1) {
  //      GetLedSection(DmaChannel, Section)->LedColorIndex = 1;
  //      UpdateUsb = 1;
  //    } else if (ButtonState == 0) {
  //      GetLedSection(DmaChannel, Section)->LedColorIndex = 0;
  //      UpdateUsb = 1;
  //    }
  //  }
  //
  //  if (UpdateUsb) {
  // uint8_t message[] = "USB update;";
  // HAL_UART_Transmit(&huart1, message, sizeof(message), 100);
  // USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, (uint8_t*)&gButtonState, sizeof(gButtonState));
  // if (hdma_tim1_ch1.State == HAL_DMA_STATE_READY) {
  //   if (InitializeMemoryForDmaTransaction(gLedCh1Buffer, 8, 0, &gLedCh1BufferIndex, &gLedCh1LedIndex, &gLedCh1SectionIndex) != 0) {
  //     // uint8_t message[] = "d1;";
  //     // HAL_UART_Transmit(&huart1, message, sizeof(message), 100);
  //     HAL_TIM_PWM_Start_DMA (&htim1, TIM_CHANNEL_1, (uint32_t*)gLedCh1Buffer, 24 * 8);
  //   }
  // }
  // if (hdma_tim1_ch2.State == HAL_DMA_STATE_READY) {
  //   if (InitializeMemoryForDmaTransaction(gLedCh2Buffer, 8, 1, &gLedCh2BufferIndex, &gLedCh2LedIndex, &gLedCh2SectionIndex) != 0) {
  //       // uint8_t message[] = "d2;";
  //       // HAL_UART_Transmit(&huart1, message, sizeof(message), 100);
  //     HAL_TIM_PWM_Start_DMA (&htim1, TIM_CHANNEL_2, (uint32_t*)gLedCh2Buffer, 24 * 8);
  //   }
  // }
  //  }
}

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
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  MX_USB_DEVICE_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  AdcInitializeData();
  InitializeConfigs(2); // 2 DMA channels
  InitializeConfig(GET_INDEX_FROM_CHANNEL(TIM_CHANNEL_1), 2, (uint8_t *){18, 18}, 24 * 8);
  InitializeConfig(GET_INDEX_FROM_CHANNEL(TIM_CHANNEL_2), 3, (uint8_t *){18, 18, 24}, 24 * 8);
  GetLedSection(0, 0)->Color = (COLOR){255, 255, 255};
  GetLedSection(0, 1)->Color = (COLOR){255, 255, 255};
  GetLedSection(0, 0)->Color = (COLOR){255, 255, 255};
  GetLedSection(0, 1)->Color = (COLOR){255, 255, 255};
  GetLedSection(0, 2)->Color = (COLOR){255, 0, 255};

  HAL_Delay(500);
  gButtonState.value = 0;
  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)gAdcBuffer, ADC_DMA_BUFFER_SIZE); // start adc in DMA mode
  PrepareBufferForTransaction(0, gLedCh1Buffer);
  HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)gLedCh1Buffer, 24 * 8);
  PrepareBufferForTransaction(1, gLedCh2Buffer);
  HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (uint32_t *)gLedCh2Buffer, 24 * 8);
  uint8_t start[] = "working";
  HAL_UART_Transmit(&huart1, start, sizeof(start), 100);

  //  HAL_PWR_EnableSleepOnExit();
  //  HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint8_t message[35] = "";
  uint8_t size = 0;
  while (1)
  {
    HAL_Delay(20);
// #define MINE_DEBUG
#ifdef MINE_DEBUG
    size = snprintf(message, 35, "up:%d,ri:%d,do:%d,le:%d\n", gAdcBuffer[0], gAdcBuffer[1], gAdcBuffer[2], gAdcBuffer[3]);
    if (size > 0)
    {
      HAL_UART_Transmit_IT(&huart1, message, size);
    }
#endif // MINE_DEBUG
    // {
    //   uint8_t AdcChannel;
    //   uint8_t DmaChannel;
    //   uint8_t Section;
    //   uint8_t ButtonState;
    //   uint8_t UpdateUsb;

    //   UpdateUsb = 0;
    //   for (AdcChannel = 0; AdcChannel < ADC_CHANNEL_COUNT; AdcChannel++)
    //   {
    //     Section = gAdcToSectionMapping[AdcChannel];
    //     DmaChannel = gAdcToChannelMapping[AdcChannel];
    //     ButtonState = GetButtonState(AdcChannel);
    //     if (ButtonState == 1)
    //     {
    //       GetLedSection(DmaChannel, Section)->Color = (COLOR){63, 236, 24};
    //       UpdateUsb = 1;
    //     }
    //     else if (ButtonState == 0)
    //     {
    //       GetLedSection(DmaChannel, Section)->Color = (COLOR){139, 0, 155};
    //       UpdateUsb = 1;
    //     }
    //   }
    //   if (UpdateUsb)
    //   {
    //     //  uint8_t message[] = "USB update;";
    //     //  HAL_UART_Transmit(&huart1, message, sizeof(message), 100);
    //     USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, (uint8_t *)&gButtonState, sizeof(gButtonState));
    //     if (hdma_tim1_ch1.State == HAL_DMA_STATE_READY)
    //     {
    //       if (PrepareBufferForTransaction(0, gLedCh1Buffer) != 0)
    //       {
    //         HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)gLedCh1Buffer, 24 * 8);
    //       }
    //       //  if (InitializeMemoryForDmaTransaction(gLedCh1Buffer, 8, 0, &gLedCh1BufferIndex, &gLedCh1LedIndex, &gLedCh1SectionIndex) != 0) {
    //       //    // uint8_t message[] = "d1;";
    //       //    // HAL_UART_Transmit(&huart1, message, sizeof(message), 100);
    //       //    HAL_TIM_PWM_Start_DMA (&htim1, TIM_CHANNEL_1, (uint32_t*)gLedCh1Buffer, 24 * 8);
    //       //  }
    //     }
    //     if (hdma_tim1_ch2.State == HAL_DMA_STATE_READY)
    //     {
    //       if (PrepareBufferForTransaction(1, gLedCh2Buffer) != 0)
    //       {
    //         HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)gLedCh1Buffer, 24 * 8);
    //       }
    //       //  if (InitializeMemoryForDmaTransaction(gLedCh2Buffer, 8, 1, &gLedCh2BufferIndex, &gLedCh2LedIndex, &gLedCh2SectionIndex) != 0) {
    //       //      // uint8_t message[] = "d2;";
    //       //      // HAL_UART_Transmit(&huart1, message, sizeof(message), 100);
    //       //    HAL_TIM_PWM_Start_DMA (&htim1, TIM_CHANNEL_2, (uint32_t*)gLedCh2Buffer, 24 * 8);
    //       //  }
    //     }
    //   }
    // }
    //   gButtonState.value = 0b0101;
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_USB;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 4;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 90;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

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
  huart1.Init.BaudRate = 250000;
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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
