
#include <memory.h>
#include <malloc.h>
#include <stdint.h>

#include "TempLeds.h"

const uint8_t ZeroOneMapping[] = {30, 60};

#define BITS_PER_BYTE 8
#define LED_TRANSFER_SIZE (BITS_PER_BYTE * 3)

const COLOR gColor[] = {{0xFF, 0xFF, 0xFF}, {0, 0, 0xFF}, {0, 0xFF, 0}, {0xFF, 0, 0}};

typedef struct {
  LED_SECTION *LedSections;
  uint8_t LedSectionsSize;
} smth;

smth *Channels;
smth *ChannelsShadow;
uint8_t ChannelsSize;

LED_SECTION *GetLedSection(uint8_t ChannelIndex, uint8_t SectionIndex) {
  return &Channels[ChannelIndex].LedSections[SectionIndex];
}

void Initialize(uint8_t NumberOfChannels) {
  Channels = malloc(sizeof(smth) * NumberOfChannels);
  ChannelsShadow = malloc(sizeof(smth) * NumberOfChannels);
  ChannelsSize = NumberOfChannels;
}

void InitializeChannel(uint8_t ChannelIndex, uint8_t NumberOfSections) {
  Channels[ChannelIndex].LedSections = malloc(sizeof(LED_SECTION) * NumberOfSections);
  ChannelsShadow[ChannelIndex].LedSections = malloc(sizeof(LED_SECTION) * NumberOfSections);
  memset(Channels[ChannelIndex].LedSections, 0, sizeof(LED_SECTION) * NumberOfSections);
  Channels[ChannelIndex].LedSectionsSize = NumberOfSections;
  ChannelsShadow[ChannelIndex].LedSectionsSize = NumberOfSections;
}

uint8_t FillBuffer(uint16_t *Buffer, uint8_t BufferStartIndex, uint8_t ChannelIndex, uint8_t *SectionStartIndex, uint8_t *LedStartIndex, uint8_t TransferSizeInLeds) {
  uint8_t *Index;
  uint8_t *LedIndex;
  uint8_t BufferIndex;
  uint8_t *ColorAddr;
  uint8_t BitInTransferIndex;
  uint8_t TransactionCounter;
  uint8_t LedTransactionCounter = 0;

  LedIndex = LedStartIndex;
  for (Index = SectionStartIndex; *Index < Channels[ChannelIndex].LedSectionsSize; *Index = *Index + 1) {
    for (; *LedIndex < Channels[ChannelIndex].LedSections[*Index].LedAmount; *LedIndex = *LedIndex + 1) {
      TransactionCounter = 0;
      for (BufferIndex = BufferStartIndex; BufferIndex < BufferStartIndex + LED_TRANSFER_SIZE; BufferIndex++) {
        ColorAddr = (uint8_t*)&gColor[Channels[ChannelIndex].LedSections[*Index].LedColorIndex];
        BitInTransferIndex = BufferIndex % BITS_PER_BYTE;
        ColorAddr = ColorAddr + TransactionCounter / BITS_PER_BYTE;
        Buffer[BufferIndex] = ZeroOneMapping[(((*(ColorAddr)) * 20) / 100) >> (7 - (BitInTransferIndex)) & 0x1];
        TransactionCounter++;
        TransactionCounter = TransactionCounter % LED_TRANSFER_SIZE;
      }
      BufferStartIndex += LED_TRANSFER_SIZE;
      LedTransactionCounter++;
      if (LedTransactionCounter == TransferSizeInLeds) {
        *LedIndex = *LedIndex + 1;
        return 0;
      }
    }
    *LedIndex = 0;
  }
  return *Index >= Channels[ChannelIndex].LedSectionsSize;
}

uint8_t InitializeMemoryForDmaTransaction(uint16_t *Buffer, uint8_t BufferSizeInLeds, uint8_t ChannelIndex, uint8_t *BufferIndex, uint8_t *LedIndex, uint8_t *SectionIndex) {
  *BufferIndex = 0;
  *SectionIndex = 0;
  *LedIndex = 0;

  if (memcmp(Channels[ChannelIndex].LedSections, ChannelsShadow[ChannelIndex].LedSections, sizeof(LED_SECTION) * Channels[ChannelIndex].LedSectionsSize) == 0) {
    return 0;
  }

  memcpy(ChannelsShadow[ChannelIndex].LedSections, Channels[ChannelIndex].LedSections, sizeof(LED_SECTION) * Channels[ChannelIndex].LedSectionsSize);
  FillBuffer(Buffer, *BufferIndex, ChannelIndex, SectionIndex, LedIndex, BufferSizeInLeds);
  return 1;
}

uint8_t HandleDmaCircularMode(uint16_t *Buffer, uint8_t BufferSizeInLeds, uint8_t ChannelIndex, uint8_t *BufferIndex, uint8_t *LedIndex, uint8_t *SectionIndex) {
  uint8_t HalfOfBufferSize = BufferSizeInLeds / 2;
  uint8_t ReturnValue;
  ReturnValue = FillBuffer(Buffer, *BufferIndex, ChannelIndex, SectionIndex, LedIndex, HalfOfBufferSize);
  *BufferIndex = (*BufferIndex + (HalfOfBufferSize * LED_TRANSFER_SIZE)) % (BufferSizeInLeds * LED_TRANSFER_SIZE);
  return ReturnValue;
}
