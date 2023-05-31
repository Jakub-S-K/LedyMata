
#ifndef _LEDS_H_
#define _LEDS_H_

#include <stdint.h>

typedef struct
{
  uint8_t LedAmount;
  uint8_t LedColorIndex;
  uint8_t Brightness;
} LED_SECTION;

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

LED_SECTION *GetLedSection(uint8_t ChannelIndex, uint8_t SectionIndex);
void InitializeLedConfigs(
    const uint8_t NumberOfLedConfigurations);
void InitializeLedSection(const uint8_t LedConfigIndex,
                          const uint8_t NumberOfSections);
uint8_t InitializeMemoryForDmaTransaction(
    const uint8_t LedConfigIndex,
    uint16_t *const Buffer,
    const uint8_t AmountOfLedsInBuffer,
    uint8_t *const BufferIndex,
    uint8_t *const LedIndex,
    uint8_t *const SectionIndex);
uint8_t HandleDmaCircularMode(
    const uint8_t LedConfigIndex,
    uint16_t *const Buffer,
    const uint8_t AmountOfLedsInBuffer,
    uint8_t *const BufferIndex,
    uint8_t *const LedIndex,
    uint8_t *const SectionIndex);

#endif // _LEDS_H_
