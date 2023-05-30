#ifndef __LEDS_H__
#define __LEDS_H__

#include "main.h"

//
// Buffer count MUST be even number
//
#define LED_DMA_BUFFER_COUNT 8
#define LED_DMA_BUFFER_SIZE (LED_BIT_COUNT * LED_DMA_BUFFER_COUNT)

#define SECTION_INDEX_DOWN 0
#define SECTION_INDEX_LEFT 1
#define SECTION_INDEX_CENTER 2

#define SECTION_INDEX_UP 0
#define SECTION_INDEX_RIGHT 1

#define COLOR_INDEX_WHITE 0
#define COLOR_INDEX_BLUE 1
#define COLOR_INDEX_RED 2
#define COLOR_INDEX_GREEN 3

#define LED_CH1_SECTION_COUNT 3
#define LED_CH2_SECTION_COUNT 2

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
HAL_StatusTypeDef LedTransferColorsBySectionsCh1(TIM_HandleTypeDef *htim, uint8_t *SectionColors);
HAL_StatusTypeDef LedTransferColorsBySectionsCh2(TIM_HandleTypeDef *htim, uint8_t *SectionColors);
void LedHandleDmaCh1Callback(TIM_HandleTypeDef *htim);
void LedHandleDmaCh2Callback(TIM_HandleTypeDef *htim);

#endif // __LEDS_H__