#include "Leds.h"

#include <memory.h>
#include <stdint.h>

#define SECTION_UP_LED_COUNT 18
#define SECTION_DOWN_LED_COUNT 18
#define SECTION_LEFT_LED_COUNT 18
#define SECTION_RIGHT_LED_COUNT 18
#define SECTION_CENTER_LED_COUNT 24
#define LED_COUNT_CH_1 (SECTION_DOWN_LED_COUNT + SECTION_LEFT_LED_COUNT + SECTION_CENTER_LED_COUNT)
#define LED_COUNT_CH_2 (SECTION_UP_LED_COUNT + SECTION_RIGHT_LED_COUNT)

#define LED_BIT_COUNT 24
#define LED_BYTE_COUNT (LED_BIT_COUNT / 8)

#define LED_CH1_SECTION_COUNT 3
#define LED_CH2_SECTION_COUNT 2

typedef struct
{
  uint8_t Green;
  uint8_t Red;
  uint8_t Blue;
} COLOR;

const COLOR gColorTable[] = {{0xFF, 0xFF, 0xFF}, {0, 0, 0xFF}, {0, 0xFF, 0}, {0xFF, 0, 0}};
uint16_t gLedBufferCh1[LED_DMA_BUFFER_SIZE];
uint16_t gLedBufferCh2[LED_DMA_BUFFER_SIZE];
uint8_t gSectionCh1Colors[LED_CH1_SECTION_COUNT];
uint8_t gSectionCh2Colors[LED_CH2_SECTION_COUNT];
uint8_t gLedCh1SectionLedCounts[LED_CH1_SECTION_COUNT];
uint8_t gLedCh2SectionLedCounts[LED_CH2_SECTION_COUNT];
uint16_t gLedBufferCh1Index;
uint16_t gLedBufferCh2Index;
uint8_t gLedDmaCh1Index;
uint8_t gLedDmaCh2Index;
uint8_t gStopLedDmaCh1Transffer;
uint8_t gStopLedDmaCh2Transffer;

uint16_t GetCurrentSectionIndex(uint16_t LedIndex, uint8_t Channel)
{
  uint8_t Index;
  uint16_t MinLedIndex = 0;
  uint16_t MaxLedIndex;
  uint16_t MaxIndex;
  uint8_t *SectionCount;

  if (Channel == TIM_CHANNEL_1) {
    MaxIndex = LED_CH1_SECTION_COUNT;
    SectionCount = gLedCh1SectionLedCounts;
  } else {
    MaxIndex = LED_CH2_SECTION_COUNT;
    SectionCount = gLedCh2SectionLedCounts;
  }

  MaxLedIndex = SectionCount[0];
  for (Index = 0; Index < MaxIndex; Index++)
  {

    if (LedIndex >= MinLedIndex && LedIndex < MaxLedIndex)
    {
      return Index;
    }

    if (Index + 1 == MaxIndex) {
      return 0xFF;
    }

    MinLedIndex += SectionCount[Index];
    MaxLedIndex += SectionCount[Index + 1];
  }

  return 0xFF;
}

HAL_StatusTypeDef PrepareBufferForSingleLed(uint16_t LedIndex, uint8_t LedDmaIndex, uint8_t Channel, uint16_t* LedBuffer, uint8_t* SectionColor)
{
  uint8_t SectionIndex = GetCurrentSectionIndex(LedIndex, Channel);
  uint8_t DmaBufferIndex;
  uint8_t *ColorTable;
  uint8_t ColorValue;

  if (SectionIndex == 0xFF) {
    return HAL_ERROR;
  }

  ColorTable = (uint8_t *)(&gColorTable[SectionColor[SectionIndex]]);
  for (DmaBufferIndex = LED_BIT_COUNT * LedDmaIndex; DmaBufferIndex < LED_BIT_COUNT * LedDmaIndex + LED_BIT_COUNT; DmaBufferIndex++)
  {
    ColorValue = *(ColorTable + ((DmaBufferIndex % LED_BIT_COUNT) / 8));
    ColorValue = (ColorValue * LED_BRIGHTNESS) / 100;
    ColorValue = ColorValue >> (7 - (DmaBufferIndex % 8));
    ColorValue = ((ColorValue & 0x1) * 30) + 30;
    LedBuffer[DmaBufferIndex] = ColorValue;
  }

  return HAL_OK;
}

HAL_StatusTypeDef LedTransferColorsBySectionsCh1(TIM_HandleTypeDef *htim, uint8_t *SectionColors)
{
  uint8_t Index;

  if (memcmp(SectionColors, gSectionCh1Colors, sizeof(gSectionCh1Colors)) == 0) {
    return HAL_OK;
  }

  //
  // It indicates that there is ongoing transfer
  //
  if (gLedBufferCh1Index < LED_COUNT_CH_1)
  {
    return HAL_BUSY;
  }

  memcpy(gSectionCh1Colors, SectionColors, sizeof(gSectionCh1Colors));
  gLedDmaCh1Index = 0;
  gLedBufferCh1Index = 0;
  for (Index = 0; Index < LED_DMA_BUFFER_COUNT; Index++)
  {
    if (PrepareBufferForSingleLed(gLedBufferCh1Index, gLedDmaCh1Index, TIM_CHANNEL_1, gLedBufferCh1, gSectionCh1Colors) != HAL_OK)
    {
      return HAL_ERROR;
    }
    gLedDmaCh1Index = (gLedDmaCh1Index + 1) % LED_DMA_BUFFER_COUNT;
    gLedBufferCh1Index++;
  }

  HAL_TIM_PWM_Start_DMA(htim, TIM_CHANNEL_1, (uint32_t*)gLedBufferCh1, LED_DMA_BUFFER_SIZE);
  return HAL_OK;
}

HAL_StatusTypeDef LedTransferColorsBySectionsCh2(TIM_HandleTypeDef *htim, uint8_t *SectionColors)
{
  uint8_t Index;

  if (memcmp(SectionColors, gSectionCh2Colors, sizeof(gSectionCh2Colors)) == 0) {
    return HAL_OK;
  }

  //
  // It indicates that there is ongoing transfer
  //
  if (gLedBufferCh1Index < LED_COUNT_CH_2)
  {
    return HAL_BUSY;
  }

  memcpy(gSectionCh2Colors, SectionColors, sizeof(gSectionCh2Colors));
  gLedDmaCh2Index = 0;
  gLedBufferCh2Index = 0;
  for (Index = 0; Index < LED_DMA_BUFFER_COUNT; Index++)
  {
    if (PrepareBufferForSingleLed(gLedBufferCh2Index, gLedDmaCh2Index, TIM_CHANNEL_2, gLedBufferCh2, gSectionCh2Colors) != HAL_OK)
    {
      return HAL_ERROR;
    }
    gLedDmaCh2Index = (gLedDmaCh2Index + 1) % LED_DMA_BUFFER_COUNT;
    gLedBufferCh2Index++;
  }

  HAL_TIM_PWM_Start_DMA(htim, TIM_CHANNEL_2, (uint32_t*)gLedBufferCh2, LED_DMA_BUFFER_SIZE);
  return HAL_OK;
}

void LedInitializeDataForDma()
{
  memset(gLedBufferCh1, 0, sizeof(gLedBufferCh1));
  memset(gLedBufferCh2, 0, sizeof(gLedBufferCh2));
  memset(gSectionCh1Colors, 0, sizeof(gSectionCh1Colors));
  memset(gSectionCh2Colors, 0, sizeof(gSectionCh2Colors));
  gLedBufferCh1Index = LED_COUNT_CH_1;
  gLedBufferCh2Index = LED_COUNT_CH_2;
  gLedCh1SectionLedCounts[SECTION_INDEX_DOWN] = SECTION_DOWN_LED_COUNT;
  gLedCh1SectionLedCounts[SECTION_INDEX_LEFT] = SECTION_LEFT_LED_COUNT;
  gLedCh1SectionLedCounts[SECTION_INDEX_CENTER] = SECTION_CENTER_LED_COUNT;
  gLedCh2SectionLedCounts[SECTION_INDEX_UP] = SECTION_UP_LED_COUNT;
  gLedCh2SectionLedCounts[SECTION_INDEX_RIGHT] = SECTION_RIGHT_LED_COUNT;
  gStopLedDmaCh1Transffer = 0;
  gStopLedDmaCh2Transffer = 0;
}

void LedHandleDmaCh1Callback(TIM_HandleTypeDef *htim)
{
  uint8_t Index;

  if (gStopLedDmaCh1Transffer)
  {
    HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1);
    gStopLedDmaCh1Transffer = 0;
    gLedBufferCh1Index = LED_COUNT_CH_1;
  }

  for (Index = 0; Index < (LED_DMA_BUFFER_COUNT >> 1); Index++)
  {
    if (PrepareBufferForSingleLed(gLedBufferCh1Index, gLedDmaCh1Index, TIM_CHANNEL_1, gLedBufferCh1, gSectionCh2Colors) != HAL_OK)
    {
      gStopLedDmaCh1Transffer = 1;
      return;
    }
    gLedDmaCh1Index = (gLedDmaCh1Index + 1) % LED_DMA_BUFFER_COUNT;
    gLedBufferCh1Index++;
  }
}

void LedHandleDmaCh2Callback(TIM_HandleTypeDef *htim)
{
  uint8_t Index;

  if (gStopLedDmaCh2Transffer)
  {
    HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_2);
    gStopLedDmaCh2Transffer = 0;
    gLedBufferCh2Index = LED_COUNT_CH_2;
  }

  for (Index = 0; Index < (LED_DMA_BUFFER_COUNT >> 1); Index++)
  {
    if (PrepareBufferForSingleLed(gLedBufferCh2Index, gLedDmaCh2Index, TIM_CHANNEL_2, gLedBufferCh2, gSectionCh2Colors) != HAL_OK)
    {
      gStopLedDmaCh2Transffer = 1;
      return;
    }
    gLedDmaCh2Index = (gLedDmaCh2Index + 1) % LED_DMA_BUFFER_COUNT;
    gLedBufferCh2Index++;
  }
}
