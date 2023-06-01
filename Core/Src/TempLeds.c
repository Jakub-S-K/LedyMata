#include <memory.h>
#include <malloc.h>
#include <stdint.h>

#include "TempLeds.h"

const uint8_t ZeroOneMapping[] = {30, 60};

#define BITS_PER_BYTE 8
#define LED_TRANSFER_SIZE (BITS_PER_BYTE * 3)
#define BRIGTHNESS 31

typedef struct
{
  LED_SECTION_COLOR *LedSections;
  uint8_t LedSectionsSize;
} LED_CONFIG;

LED_CONFIG *LedConfigs;
LED_CONFIG *LedConfigsShadow;
uint8_t NumberOfLedConfigs;

LED_SECTION_COLOR *GetLedSection(
    uint8_t LedConfigIndex,
    uint8_t SectionIndex)
{
  return &LedConfigs[LedConfigIndex].LedSections[SectionIndex];
}

void InitializeLedConfigs(
    uint8_t NumberOfLedConfigurations)
{
  LedConfigs = malloc(sizeof(LED_CONFIG) * NumberOfLedConfigurations);
  LedConfigsShadow = malloc(sizeof(LED_CONFIG) * NumberOfLedConfigurations);
  NumberOfLedConfigs = NumberOfLedConfigurations;
}

void InitializeLedSection(
    uint8_t LedConfigIndex,
    uint8_t NumberOfSections)
{
  LedConfigs[LedConfigIndex].LedSections = malloc(sizeof(LED_SECTION_COLOR) * NumberOfSections);
  LedConfigsShadow[LedConfigIndex].LedSections = malloc(sizeof(LED_SECTION_COLOR) * NumberOfSections);
  memset(LedConfigs[LedConfigIndex].LedSections, 0, sizeof(LED_SECTION_COLOR) * NumberOfSections);
  LedConfigs[LedConfigIndex].LedSectionsSize = NumberOfSections;
  LedConfigsShadow[LedConfigIndex].LedSectionsSize = NumberOfSections;
}

uint8_t FillBuffer(
    uint8_t LedConfigIndex,
    uint16_t *Buffer,
    uint8_t *BufferStartIndex,
    uint8_t *SectionStartIndex,
    uint8_t *LedStartIndex,
    uint8_t BufferSize)
{
  uint8_t *Index;
  uint8_t *LedIndex;
  uint8_t BufferIndex;
  uint8_t BufferStartTempIndex = *BufferStartIndex;
  uint8_t *ColorAddr;
  uint8_t BitInTransferIndex;
  uint8_t LedColorCounter;
  uint8_t LoopSize;
  uint8_t BufferTransactionCounter = 0;

  LedIndex = LedStartIndex;
  for (Index = SectionStartIndex; *Index < LedConfigs[LedConfigIndex].LedSectionsSize; *Index = *Index + 1)
  {
    for (; *LedIndex < LedConfigs[LedConfigIndex].LedSections[*Index].LedAmount; *LedIndex = *LedIndex + 1)
    {
      LedColorCounter = BufferStartTempIndex % LED_TRANSFER_SIZE;
      LoopSize = LedColorCounter;
      for (BufferIndex = *BufferStartIndex; BufferIndex < *BufferStartIndex + (LED_TRANSFER_SIZE - LoopSize); BufferIndex++)
      {
        ColorAddr = (uint8_t *)&(LedConfigs[LedConfigIndex].LedSections[*Index].LedColor);
        BitInTransferIndex = BufferIndex % BITS_PER_BYTE;
        ColorAddr = ColorAddr + LedColorCounter / BITS_PER_BYTE;
        Buffer[BufferIndex] = ZeroOneMapping[(((*(ColorAddr)) * BRIGTHNESS) / 100) >> (7 - (BitInTransferIndex)) & 0x1];
        LedColorCounter++;
        LedColorCounter = LedColorCounter % LED_TRANSFER_SIZE;
        BufferTransactionCounter++;
      }
      BufferStartTempIndex += LED_TRANSFER_SIZE - (BufferIndex % LED_TRANSFER_SIZE);

      if (BufferTransactionCounter == BufferSize)
      {
        if (BufferTransactionCounter % LED_TRANSFER_SIZE == 0)
        {
          *LedIndex = *LedIndex + 1;
        }
        *BufferStartIndex += BufferTransactionCounter % LED_TRANSFER_SIZE;
        return 0;
      }
    }
    *LedIndex = 0;
  }
  *BufferStartIndex = BufferIndex;
  return *Index >= LedConfigs[LedConfigIndex].LedSectionsSize;
}

uint8_t InitializeMemoryForDmaTransaction(
    uint8_t LedConfigIndex,
    uint16_t *Buffer,
    uint8_t AmountOfLedsInBuffer,
    uint8_t *BufferIndex,
    uint8_t *LedIndex,
    uint8_t *SectionIndex)
{
  *BufferIndex = 0;
  *SectionIndex = 0;
  *LedIndex = 0;

  if (memcmp(LedConfigs[LedConfigIndex].LedSections, LedConfigsShadow[LedConfigIndex].LedSections, sizeof(LED_SECTION_COLOR) * LedConfigs[LedConfigIndex].LedSectionsSize) == 0)
  {
    return 0;
  }

  memcpy(LedConfigsShadow[LedConfigIndex].LedSections, LedConfigs[LedConfigIndex].LedSections, sizeof(LED_SECTION_COLOR) * LedConfigs[LedConfigIndex].LedSectionsSize);
  FillBuffer(LedConfigIndex, Buffer, BufferIndex, SectionIndex, LedIndex, AmountOfLedsInBuffer);
  return 1;
}

uint8_t HandleDmaCircularMode(
    uint8_t LedConfigIndex,
    uint16_t *Buffer,
    uint8_t AmountOfLedsInBuffer,
    uint8_t *BufferIndex,
    uint8_t *LedIndex,
    uint8_t *SectionIndex)
{
  uint8_t HalfOfBufferSize = AmountOfLedsInBuffer / 2;
  uint8_t ReturnValue = FillBuffer(LedConfigIndex, Buffer, BufferIndex, SectionIndex, LedIndex, HalfOfBufferSize);
  return ReturnValue;
}

void InitializeForFillingWithZeroes(
    uint8_t *ZeroLedIndex)
{
  *ZeroLedIndex = 0;
}

uint8_t HandleDmaCircularModeFillWithZeroes(
    uint8_t LedConfigIndex,
    uint16_t *Buffer,
    uint8_t AmountOfLedsInBuffer,
    uint8_t *BufferIndex,
    uint8_t *ZeroLedIndex)
{
  uint8_t HalfOfBufferSize;
  uint8_t Index;

  if (*ZeroLedIndex >= AmountOfLedsInBuffer)
  {
    return 1;
  }

  HalfOfBufferSize = AmountOfLedsInBuffer / 2;

  for (Index = *BufferIndex; Index < *BufferIndex + (LED_TRANSFER_SIZE * HalfOfBufferSize); Index++)
  {
    Buffer[Index] = 0;
  }

  *ZeroLedIndex += HalfOfBufferSize;

  return 0;
}
