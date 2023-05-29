#include "Leds.h"

#include <memory.h>

#define SECTION_UP_LED_COUNT 18
#define SECTION_DOWN_LED_COUNT 18
#define SECTION_LEFT_LED_COUNT 18
#define SECTION_RIGHT_LED_COUNT 18
#define SECTION_CENTER_LED_COUNT 24
#define LED_COUNT (SECTION_UP_LED_COUNT + SECTION_DOWN_LED_COUNT + SECTION_LEFT_LED_COUNT + SECTION_RIGHT_LED_COUNT + SECTION_CENTER_LED_COUNT)

typedef struct
{
  uint8_t Green;
  uint8_t Red;
  uint8_t Blue;
} COLOR;

uint16_t gLedBuffer[LED_DMA_BUFFER_SIZE];
uint8_t gSectionColorIndex[SECTION_COUNT];
SECTION_COLORS gSectionColors;
COLOR gColorTable[] = {{0xFF, 0xFF, 0xFF}, {0, 0, 0xFF}, {0, 0xFF, 0}, {0xFF, 0, 0}};
uint8_t gSectionLedCounts[SECTION_COUNT];
uint16_t gLedIndex;
uint8_t gLedDmaIndex;
uint8_t gStopLedDmaTransffer;

uint16_t GetCurrentSectionIndex(uint16_t LedIndex)
{
  uint8_t Index = 0;
  uint16_t MinLedIndex = 0;
  uint16_t MaxLedIndex = gSectionLedCounts[Index];
  for (; Index < SECTION_COUNT; Index++)
  {

    if (LedIndex >= MinLedIndex && LedIndex < MaxLedIndex)
    {
      return Index;
    }

    if (Index + 1 == SECTION_COUNT) {
      return 0xFF;
    }

    MinLedIndex += gSectionLedCounts[Index];
    MaxLedIndex += gSectionLedCounts[Index + 1];
  }
  return 0xFF;
}

HAL_StatusTypeDef PrepareBufferForSingleLed(uint16_t LedIndex, uint8_t LedDmaIndex)
{
  uint8_t SectionIndex = GetCurrentSectionIndex(gLedIndex);
  uint8_t DmaBufferIndex;
  uint8_t *ColorTable;
  uint8_t ColorValue;

  switch (SectionIndex)
  {
  case SECTION_INDEX_UP:
    ColorTable = (uint8_t *)(&gColorTable[gSectionColors.UpColor]);
    break;
  case SECTION_INDEX_DOWN:
    ColorTable = (uint8_t *)(&gColorTable[gSectionColors.DownColor]);
    break;
  case SECTION_INDEX_LEFT:
    ColorTable = (uint8_t *)(&gColorTable[gSectionColors.LeftColor]);
    break;
  case SECTION_INDEX_RIGHT:
    ColorTable = (uint8_t *)(&gColorTable[gSectionColors.RightColor]);
    break;
  case SECTION_INDEX_CENTER:
    ColorTable = (uint8_t *)(&gColorTable[gSectionColors.CenterColor]);
    break;
  //
  // All the data was transfered
  //
  default:
    return HAL_ERROR;
    break;
  }

  for (DmaBufferIndex = LED_BIT_COUNT * LedDmaIndex; DmaBufferIndex < LED_BIT_COUNT * LedDmaIndex + LED_BIT_COUNT; DmaBufferIndex++)
  {
    ColorValue = *(ColorTable + ((DmaBufferIndex % LED_BIT_COUNT) / 8));
    ColorValue = (ColorValue * LED_BRIGHTNESS) / 100;
    ColorValue = ColorValue >> (7 - (DmaBufferIndex % 8));
    ColorValue = ((ColorValue & 0x1) * 30) + 30;
    gLedBuffer[DmaBufferIndex] = ColorValue;
  }

  return HAL_OK;
}

void LedPrepareDmaColorStruct(SECTION_COLORS *Colors, uint8_t SectionIndex, uint8_t ColorIndex)
{
  switch (SectionIndex)
  {
  case SECTION_INDEX_UP:
    Colors->UpColor = ColorIndex;
    break;
  case SECTION_INDEX_DOWN:
    Colors->DownColor = ColorIndex;
    break;
  case SECTION_INDEX_LEFT:
    Colors->LeftColor = ColorIndex;
    break;
  case SECTION_INDEX_RIGHT:
    Colors->RightColor = ColorIndex;
    break;
  case SECTION_INDEX_CENTER:
    Colors->CenterColor = ColorIndex;
    break;
  }
}

HAL_StatusTypeDef LedTransferColorsBySections(TIM_HandleTypeDef *htim, SECTION_COLORS *SectionColors)
{
  uint8_t Index;

  if (memcmp(SectionColors, &gSectionColors, sizeof(SECTION_COLORS)) == 0) {
    return HAL_OK;
  }
  //
  // It indicates that there is ongoing transfer
  //
  if (gLedIndex < LED_COUNT)
  {
    return HAL_BUSY;
  }

  memcpy(&gSectionColors, SectionColors, sizeof(SECTION_COLORS));
  gLedDmaIndex = 0;
  gLedIndex = 0;
  for (Index = 0; Index < LED_DMA_BUFFER_COUNT; Index++)
  {
    if (PrepareBufferForSingleLed(gLedIndex, gLedDmaIndex) != HAL_OK)
    {
      return HAL_ERROR;
    }
    gLedDmaIndex = (gLedDmaIndex + 1) % LED_DMA_BUFFER_COUNT;
    gLedIndex++;
  }

  HAL_TIM_PWM_Start_DMA(htim, TIM_CHANNEL_1, (uint32_t*)gLedBuffer, LED_DMA_BUFFER_SIZE);
  return HAL_OK;
}

void LedInitializeDataForDma()
{
  memset(&gSectionColors, 0, sizeof(SECTION_COLORS));
  gLedIndex = LED_COUNT;
  gSectionLedCounts[SECTION_INDEX_UP] = SECTION_UP_LED_COUNT;
  gSectionLedCounts[SECTION_INDEX_DOWN] = SECTION_DOWN_LED_COUNT;
  gSectionLedCounts[SECTION_INDEX_LEFT] = SECTION_LEFT_LED_COUNT;
  gSectionLedCounts[SECTION_INDEX_RIGHT] = SECTION_RIGHT_LED_COUNT;
  gSectionLedCounts[SECTION_INDEX_CENTER] = SECTION_CENTER_LED_COUNT;
  gStopLedDmaTransffer = 0;
}

void LedHandleDmaCallback(TIM_HandleTypeDef *htim)
{
  uint8_t Index;

  if (gStopLedDmaTransffer)
  {
    HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1);
    gStopLedDmaTransffer = 0;
    gLedIndex = LED_COUNT;
  }

  for (Index = 0; Index < (LED_DMA_BUFFER_COUNT >> 1); Index++)
  {
    if (PrepareBufferForSingleLed(gLedIndex, gLedDmaIndex) != HAL_OK)
    {
      gStopLedDmaTransffer = 1;
      return;
    }
    gLedDmaIndex = (gLedDmaIndex + 1) % LED_DMA_BUFFER_COUNT;
    gLedIndex++;
  }
}
