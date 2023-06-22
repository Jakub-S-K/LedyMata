#ifndef DUPA
#define DUPA

typedef struct
{
  uint8_t Green;
  uint8_t Red;
  uint8_t Blue;
} COLOR;

typedef struct
{
  uint8_t LedCount;
  COLOR Color;
  uint8_t TemperatureIndex;
} LED_SECTION;

void InitializeConfigs(uint8_t AmountOfConfigs);
uint8_t InitializeConfig(uint8_t ConfigIndex, uint8_t AmountOfSections, uint8_t *LedCounts, uint8_t BufferSize);
LED_SECTION *GetLedSection(uint8_t ConfigIndex, uint8_t SectionIndex);
uint8_t FillHalfBuffer(uint8_t ConfigIndex, uint16_t *Buffer);
uint8_t FillHalfBufferWithZeros(uint8_t ConfigIndex, uint16_t *Buffer);
uint8_t PrepareBufferForTransaction(uint8_t ConfigIndex, uint8_t *Buffer);

#endif