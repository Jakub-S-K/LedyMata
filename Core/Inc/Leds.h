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

typedef struct
{
  uint8_t Green;
  uint8_t Red;
  uint8_t Blue;
} COLOR;

#define LED_BIT_COUNT 24
#define LED_BYTE_COUNT (LED_BIT_COUNT / 8)

//
// Buffer count MUST be even number
//
#define LED_DMA_BUFFER_COUNT 4
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

#define SECTION_UP_LED_COUNT 18
#define SECTION_DOWN_LED_COUNT 18
#define SECTION_LEFT_LED_COUNT 18
#define SECTION_RIGHT_LED_COUNT 18
#define SECTION_CENTER_LED_COUNT 24
#define LED_COUNT (SECTION_UP_LED_COUNT + SECTION_DOWN_LED_COUNT + SECTION_LEFT_LED_COUNT + SECTION_RIGHT_LED_COUNT + SECTION_CENTER_LED_COUNT)

//
// In percents
//
#define LED_BRIGHTNESS 20

//
// Before calling any other function call prepare data first
//
void LedPrepareDataForDma();

//
// To easily change colors of each section call this function to prepare struct for each section
//
void LedPrepareDmaStructStruct(SECTION_COLORS *Colors, uint8_t SectionIndex, uint8_t ColorIndex);
HAL_StatusTypeDef LedTransferColorsBySections(TIM_HandleTypeDef *htim, SECTION_COLORS *SectionColors);
void LedHandleDmaCallback(TIM_HandleTypeDef *htim);

#endif // __LEDS_H__