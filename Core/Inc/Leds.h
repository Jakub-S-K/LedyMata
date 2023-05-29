#ifndef __LEDS_H__
#define __LEDS_H__

#include "main.h"

typedef struct
{
  uint8_t UpColor;
  uint8_t DownColor;
  uint8_t LeftColor;
  uint8_t RightColor;
  uint8_t CenterColor;
} SECTION_COLORS;

#define LED_BIT_COUNT 24
#define LED_BYTE_COUNT (LED_BIT_COUNT / 8)

//
// Buffer count MUST be even number
//
#define LED_DMA_BUFFER_COUNT 8
#define LED_DMA_BUFFER_SIZE (LED_BIT_COUNT * LED_DMA_BUFFER_COUNT)

#define SECTION_COUNT 5

#define SECTION_INDEX_UP 0
#define SECTION_INDEX_DOWN 1
#define SECTION_INDEX_LEFT 2
#define SECTION_INDEX_RIGHT 3
#define SECTION_INDEX_CENTER 4

#define COLOR_INDEX_WHITE 0
#define COLOR_INDEX_BLUE 1
#define COLOR_INDEX_RED 2
#define COLOR_INDEX_GREEN 3

//
// In percents
//
#define LED_BRIGHTNESS 25

//
// Before calling any other function call prepare data first
//
void LedInitializeDataForDma();

//
// To easily change colors of each section call this function to prepare struct for each section
//
void LedPrepareDmaColorStruct(SECTION_COLORS *Colors, uint8_t SectionIndex, uint8_t ColorIndex);
HAL_StatusTypeDef LedTransferColorsBySections(TIM_HandleTypeDef *htim, SECTION_COLORS *SectionColors);
void LedHandleDmaCallback(TIM_HandleTypeDef *htim);

#endif // __LEDS_H__