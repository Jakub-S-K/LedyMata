
#ifndef DUPA
#define DUPA

#include <stdint.h>

typedef struct {
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

typedef struct {
  uint8_t LedAmount;
  COLOR   LedColor;
  uint8_t Brightness;
} LED_SECTION_COLOR;

LED_SECTION *GetLedSection(uint8_t ChannelIndex, uint8_t SectionIndex);
void Initialize(uint8_t NumberOfChannels);
void InitializeChannel(uint8_t ChannelIndex, uint8_t NumberOfSections);
uint8_t InitializeMemoryForDmaTransaction(uint16_t *Buffer, uint8_t BufferSizeInLeds, uint8_t ChannelIndex, uint8_t *BufferIndex, uint8_t *LedIndex, uint8_t *SectionIndex);
uint8_t HandleDmaCircularMode(uint16_t *Buffer, uint8_t BufferSizeInLeds, uint8_t ChannelIndex, uint8_t *BufferIndex, uint8_t *LedIndex, uint8_t *SectionIndex);

#endif // DUPA
