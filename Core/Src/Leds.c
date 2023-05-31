
#include <memory.h>
#include <malloc.h>
#include <stdint.h>

#include "Leds.h"

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
    const uint8_t LedConfigIndex,
    const uint8_t SectionIndex)
{
  return &LedConfigs[LedConfigIndex].LedSections[SectionIndex];
}

void InitializeLedConfigs(
    const uint8_t NumberOfLedConfigurations)
{
  LedConfigs = malloc(sizeof(LED_CONFIG) * NumberOfLedConfigurations);
  LedConfigsShadow = malloc(sizeof(LED_CONFIG) * NumberOfLedConfigurations);
  NumberOfLedConfigs = NumberOfLedConfigurations;
}

void InitializeLedSection(
    const uint8_t LedConfigIndex,
    const uint8_t NumberOfSections)
{
  LedConfigs[LedConfigIndex].LedSections = malloc(sizeof(LED_SECTION_COLOR) * NumberOfSections);
  LedConfigsShadow[LedConfigIndex].LedSections = malloc(sizeof(LED_SECTION_COLOR) * NumberOfSections);
  memset(LedConfigs[LedConfigIndex].LedSections, 0, sizeof(LED_SECTION_COLOR) * NumberOfSections);
  LedConfigs[LedConfigIndex].LedSectionsSize = NumberOfSections;
  LedConfigsShadow[LedConfigIndex].LedSectionsSize = NumberOfSections;
}

uint8_t FillBuffer(
    const uint8_t LedConfigIndex,
    uint16_t *const Buffer,
    const uint8_t BufferStartIndex,
    uint8_t *const SectionStartIndex,
    uint8_t *const LedStartIndex,
    const uint8_t TransferSizeInLeds)
{
  uint8_t *Index;
  uint8_t *LedIndex;
  uint8_t BufferIndex;
  uint8_t BufferStartTempIndex = BufferStartIndex;
  uint8_t *ColorAddr;
  uint8_t BitInTransferIndex;
  uint8_t TransactionCounter;
  uint8_t LedTransactionCounter = 0;

  LedIndex = LedStartIndex;
  for (Index = SectionStartIndex; *Index < LedConfigs[LedConfigIndex].LedSectionsSize; *Index = *Index + 1)
  {
    for (; *LedIndex < LedConfigs[LedConfigIndex].LedSections[*Index].LedAmount; *LedIndex = *LedIndex + 1)
    {
      TransactionCounter = 0;
      ColorAddr = (uint8_t*)&(LedConfigs[LedConfigIndex].LedSections[*Index].LedColor);
      for (BufferIndex = BufferStartIndex; BufferIndex < BufferStartIndex + LED_TRANSFER_SIZE; BufferIndex++)
      {
        BitInTransferIndex = BufferIndex % BITS_PER_BYTE;
        ColorAddr = ColorAddr + TransactionCounter / BITS_PER_BYTE;
        Buffer[BufferIndex] = ZeroOneMapping[(((*(ColorAddr)) * BRIGTHNESS) / 100) >> (7 - (BitInTransferIndex)) & 0x1];
        TransactionCounter++;
        TransactionCounter = TransactionCounter % LED_TRANSFER_SIZE;
      }
      BufferStartTempIndex += LED_TRANSFER_SIZE;
      LedTransactionCounter++;
      if (LedTransactionCounter == TransferSizeInLeds)
      {
        *LedIndex = *LedIndex + 1;
        return 0;
      }
    }
    *LedIndex = 0;
  }
  return *Index >= LedConfigs[LedConfigIndex].LedSectionsSize;
}

uint8_t InitializeMemoryForDmaTransaction(
    const uint8_t LedConfigIndex,
    uint16_t *const Buffer,
    const uint8_t AmountOfLedsInBuffer,
    uint8_t *const BufferIndex,
    uint8_t *const LedIndex,
    uint8_t *const SectionIndex)
{
  if (memcmp(LedConfigs[LedConfigIndex].LedSections, LedConfigsShadow[LedConfigIndex].LedSections, sizeof(LED_SECTION_COLOR) * LedConfigs[LedConfigIndex].LedSectionsSize) == 0)
  {
    return 0;
  }

  *BufferIndex = 0;
  *SectionIndex = 0;
  *LedIndex = 0;

  memcpy(LedConfigsShadow[LedConfigIndex].LedSections, LedConfigs[LedConfigIndex].LedSections, sizeof(LED_SECTION_COLOR) * LedConfigs[LedConfigIndex].LedSectionsSize);
  FillBuffer(LedConfigIndex, Buffer, *BufferIndex, SectionIndex, LedIndex, AmountOfLedsInBuffer);
  return 1;
}

uint8_t HandleDmaCircularMode(
    const uint8_t LedConfigIndex,
    uint16_t *const Buffer,
    const uint8_t AmountOfLedsInBuffer,
    uint8_t *const BufferIndex,
    uint8_t *const LedIndex,
    uint8_t *const SectionIndex)
{
  uint8_t HalfOfBufferSize = AmountOfLedsInBuffer / 2;
  uint8_t ReturnValue = FillBuffer(LedConfigIndex, Buffer, *BufferIndex, SectionIndex, LedIndex, HalfOfBufferSize);
  *BufferIndex = (*BufferIndex + (HalfOfBufferSize * LED_TRANSFER_SIZE)) % (AmountOfLedsInBuffer * LED_TRANSFER_SIZE);
  return ReturnValue;
}
