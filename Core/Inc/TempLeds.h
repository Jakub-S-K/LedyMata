#ifndef _LEDS_H_
#define _LEDS_H_

#include <stdint.h>

typedef struct
{
  uint8_t Green;
  uint8_t Red;
  uint8_t Blue;
} COLOR;

typedef struct
{
  uint8_t LedAmount;
  COLOR LedColor;
  uint8_t Brightness;
} LED_SECTION_COLOR;

LED_SECTION_COLOR *GetLedSection(uint8_t ChannelIndex, uint8_t SectionIndex);
void InitializeLedConfigs(
    uint8_t NumberOfLedConfigurations);
void InitializeLedSection(uint8_t LedConfigIndex,
                          uint8_t NumberOfSections);
uint8_t InitializeMemoryForDmaTransaction(
    uint8_t LedConfigIndex,
    uint16_t *Buffer,
    uint8_t AmountOfLedsInBuffer,
    uint8_t *BufferIndex,
    uint8_t *LedIndex,
    uint8_t *SectionIndex);
uint8_t HandleDmaCircularMode(
    uint8_t LedConfigIndex,
    uint16_t *Buffer,
    uint8_t AmountOfLedsInBuffer,
    uint8_t *BufferIndex,
    uint8_t *LedIndex,
    uint8_t *SectionIndex);
void InitializeForFillingWithZeroes(
    uint8_t *ZeroLedIndex);
uint8_t HandleDmaCircularModeFillWithZeroes(
    uint8_t LedConfigIndex,
    uint16_t *Buffer,
    uint8_t AmountOfLedsInBuffer,
    uint8_t *BufferIndex,
    uint8_t *ZeroLedIndex);

#endif // _LEDS_H_
