#include <stdint.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

#include "dupaleds.h"

// mnożniki
static COLOR gTemperatures[] = {{255, 255, 255}};

typedef struct
{
  LED_SECTION *Sections;
  uint8_t SectionsSize;
  uint16_t BufferIndex;
  uint16_t BufferSize;
  uint8_t LedIndex;
  uint8_t SectionIndex;
} LED_CONFIG;

LED_CONFIG *gConfigs = NULL;
uint8_t gConfigsSize = 0;
uint8_t gConfigNumber = 0;

void InitializeConfigs(uint8_t AmountOfConfigs)
{
  if (gConfigs != NULL)
  {
    return;
  }

  gConfigs = malloc(sizeof(LED_CONFIG) * AmountOfConfigs);
  memset(gConfigs, 0, sizeof(LED_CONFIG) * AmountOfConfigs);
  gConfigsSize = AmountOfConfigs;
}

uint8_t InitializeConfig(uint8_t ConfigIndex, uint8_t AmountOfSections, uint8_t *LedCounts, uint8_t BufferSize)
{
  if (gConfigNumber >= gConfigsSize)
  {
    return 1;
  }

  if (gConfigs[ConfigIndex].Sections != NULL)
  {
    return 2;
  }

  gConfigs[ConfigIndex].Sections = malloc(sizeof(LED_SECTION) * AmountOfSections);
  memset(gConfigs[ConfigIndex].Sections, 0, sizeof(LED_SECTION) * AmountOfSections);
  for (int Index = 0; Index < AmountOfSections; Index++)
  {
    gConfigs[ConfigIndex].Sections[Index].LedCount = LedCounts[Index];
    gConfigs[ConfigIndex].Sections[Index].Color = (COLOR){255, 127, 50};
  }
  gConfigs[ConfigIndex].SectionsSize = AmountOfSections;
  gConfigs[ConfigIndex].BufferSize = BufferSize;
  gConfigNumber++;
  return 0;
}

#define BRIGHTNESS 100

LED_SECTION *GetLedSection(uint8_t ConfigIndex, uint8_t SectionIndex)
{
  return &gConfigs[ConfigIndex].Sections[SectionIndex];
}

uint8_t GetPwmValueFromColor(uint8_t ConfigIndex)
{
  static uint8_t PwmValue[2] = {30, 60};
  uint8_t ColorOffset = (gConfigs[ConfigIndex].BufferIndex % 24) / 8;
  uint8_t *temp = (uint8_t *)(&(gTemperatures[gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].TemperatureIndex]));
  uint8_t TemperatureValue = (*(uint8_t *)(temp + ColorOffset));
  // printf("%p %p\n", &(gTemperatures[gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].TemperatureIndex]) + ColorOffset, gTemperatures);
  // printf("%d\n", *((uint8_t*)(&(gTemperatures[gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].TemperatureIndex]))));
  uint32_t ColorByte = *(((uint8_t *)&gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].Color) + ColorOffset);
  // printf("ColorByte %d\n", ColorByte);
  ColorByte = (ColorByte * (uint32_t)(TemperatureValue * BRIGHTNESS));
  ColorByte = ColorByte / ((uint32_t)(255 * 255));
  // printf("Tak %d\n", (uint32_t)(TemperatureValue * BRIGHTNESS));
  // printf("Temperature %d\n", (TemperatureValue ));
  uint8_t ColorBitOffset = 7 - (gConfigs[ConfigIndex].BufferIndex % 8);
  // printf("ColorByte %d; ColorBitOffset %d; PwmValue %d;\n", ColorByte, ColorBitOffset, PwmValue[(ColorByte >> ColorBitOffset) & 1]);
  return PwmValue[(ColorByte >> ColorBitOffset) & 1];
}

void BufferDump(uint8_t ConfigIndex, uint16_t *Buffer)
{
  printf("Buffer dump:\n");
  for (int i = 0; i < gConfigs[ConfigIndex].BufferSize / 24; i++)
  {
    printf("[");
    for (int j = 0; j < 24; j++)
    {
      printf("%3d ", Buffer[i * 24 + j]);
    }
    printf("\b]\n");
  }
  printf("\n");
}

uint8_t FillHalfBuffer(uint8_t ConfigIndex, uint16_t *Buffer)
{
  uint16_t BufferFilled = 0;
  uint16_t BufferToBeFilled = gConfigs[ConfigIndex].BufferSize / 2;
  uint16_t BufferSizeIteration = gConfigs[ConfigIndex].BufferIndex;

  if (gConfigs[ConfigIndex].BufferIndex != BufferToBeFilled && gConfigs[ConfigIndex].BufferIndex != 0)
  {
    return 2;
  }

  for (; gConfigs[ConfigIndex].SectionIndex < gConfigs[ConfigIndex].SectionsSize; gConfigs[ConfigIndex].SectionIndex++)
  {
    printf("Entered\n");
    // printf("[%d] LedIndex %d; BufferIndex %d\n", gConfigs[ConfigIndex].SectionIndex, gConfigs[ConfigIndex].LedIndex, gConfigs[ConfigIndex].BufferIndex);
    for (; gConfigs[ConfigIndex].LedIndex < gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].LedCount; gConfigs[ConfigIndex].LedIndex++)
    {
      if (BufferFilled == BufferToBeFilled)
      {
        // printf("Exiting\n");
        break;
      }
      if (BufferFilled + 24 > BufferToBeFilled)
      {
        BufferSizeIteration += BufferToBeFilled - BufferFilled;
      }
      else
      {
        BufferSizeIteration += 24;
      }
      // printf("[%d.%d] BufferSizeIteration %d\n", gConfigs[ConfigIndex].SectionIndex, gConfigs[ConfigIndex].LedIndex, BufferSizeIteration);
      for (; gConfigs[ConfigIndex].BufferIndex < BufferSizeIteration; gConfigs[ConfigIndex].BufferIndex++, BufferFilled++)
      {
        // printf("[%d.%d.%d]\n", gConfigs[ConfigIndex].SectionIndex, gConfigs[ConfigIndex].LedIndex, gConfigs[ConfigIndex].BufferIndex);
        Buffer[gConfigs[ConfigIndex].BufferIndex] = GetPwmValueFromColor(ConfigIndex);
      }
    }
    if (BufferFilled == BufferToBeFilled)
    {
      break;
    }
    gConfigs[ConfigIndex].LedIndex = 0;
  }
  gConfigs[ConfigIndex].BufferIndex %= gConfigs[ConfigIndex].BufferSize;
  // printf("BufferIndex %d; BufferSize %d\n", gConfigs[ConfigIndex].BufferIndex, gConfigs[ConfigIndex].BufferSize);
  BufferDump(ConfigIndex, Buffer);
  return BufferFilled != BufferToBeFilled;
}

uint8_t FillHalfBufferWithZeros(uint8_t ConfigIndex, uint16_t *Buffer)
{
  uint16_t BufferFilled = 0;
  uint16_t BufferToBeFilled = gConfigs[ConfigIndex].BufferSize / 2;
  uint16_t BufferSizeIteration = gConfigs[ConfigIndex].BufferIndex;

  if (gConfigs[ConfigIndex].BufferIndex != BufferToBeFilled && gConfigs[ConfigIndex].BufferIndex != 0)
  {
    return 2;
  }

  if (gConfigs[ConfigIndex].SectionIndex - gConfigs[ConfigIndex].SectionsSize >= 2) {
   return 1;
  }

  for (; gConfigs[ConfigIndex].BufferIndex < BufferSizeIteration + BufferToBeFilled; gConfigs[ConfigIndex].BufferIndex++, BufferFilled++)
  {
    Buffer[gConfigs[ConfigIndex].BufferIndex] = 0;
  }
  gConfigs[ConfigIndex].BufferIndex %= gConfigs[ConfigIndex].BufferSize;
  gConfigs[ConfigIndex].SectionIndex++;
  return 0;
  // BufferDump(ConfigIndex, Buffer);
}

uint8_t PrepareBufferForTransaction(uint8_t ConfigIndex, uint8_t *Buffer)
{
  uint8_t ReturnValue = 0;

  gConfigs[ConfigIndex].LedIndex = 0;
  gConfigs[ConfigIndex].SectionIndex = 0;
  gConfigs[ConfigIndex].BufferIndex = 0;
  ReturnValue = FillHalfBuffer(ConfigIndex, Buffer);
  // if (ReturnValue) {
  // ReturnValue = FillHalfBufferWithZeros(ConfigIndex, Buffer);
  // }
  ReturnValue |= FillHalfBuffer(ConfigIndex, Buffer);
  return ReturnValue;
}

// int main()
// {
//   uint16_t Buffer1[24 * 8];
//   uint16_t Buffer2[24 * 8];
//   InitializeConfigs(2);
//   uint8_t leds[] = {10, 12, 14, 16};
//   InitializeConfig(0, 4, leds, 8 * 24);
//   InitializeConfig(1, 4, leds, 8 * 24);
//   PrepareBufferForTransaction(0, Buffer1);
//   PrepareBufferForTransaction(1, Buffer2);
//   printf("%d\n", FillHalfBuffer(0, Buffer1));
//   printf("%d\n", FillHalfBuffer(1, Buffer2));
//   printf("%d\n", FillHalfBuffer(0, Buffer1));
//   printf("%d\n", FillHalfBuffer(1, Buffer2));
//   printf("%d\n", FillHalfBuffer(0, Buffer1));
//   printf("%d\n", FillHalfBuffer(1, Buffer2));
//   printf("%d\n", FillHalfBuffer(0, Buffer1));
//   printf("%d\n", FillHalfBuffer(1, Buffer2));
//   printf("%d\n", FillHalfBuffer(0, Buffer1));
//   printf("%d\n", FillHalfBuffer(1, Buffer2));
//   printf("%d\n", FillHalfBuffer(0, Buffer1));
//   printf("%d\n", FillHalfBuffer(1, Buffer2));
//   printf("%d\n", FillHalfBuffer(0, Buffer1));
//   printf("%d\n", FillHalfBuffer(1, Buffer2));
//   printf("%d\n", FillHalfBuffer(0, Buffer1));
//   printf("%d\n", FillHalfBuffer(1, Buffer2));
//   printf("%d\n", FillHalfBuffer(0, Buffer1));
//   printf("%d\n", FillHalfBuffer(1, Buffer2));
//   printf("%d\n", FillHalfBuffer(0, Buffer1));
//   printf("%d\n", FillHalfBuffer(1, Buffer2));
//   printf("%d\n", FillHalfBuffer(0, Buffer1));
//   printf("%d\n", FillHalfBuffer(1, Buffer2));
//   printf("%d\n", FillHalfBuffer(0, Buffer1));
//   printf("%d\n", FillHalfBuffer(1, Buffer2));
//   printf("%d\n", FillHalfBuffer(0, Buffer1));
//   printf("%d\n", FillHalfBuffer(1, Buffer2));
//   printf("%d Zero\n", FillHalfBufferWithZeros(0, Buffer1));
//   printf("%d\n", FillHalfBufferWithZeros(1, Buffer2));
//   printf("%d\n", FillHalfBufferWithZeros(0, Buffer1));
//   printf("%d\n", FillHalfBufferWithZeros(1, Buffer2));
//   printf("%d\n", FillHalfBufferWithZeros(0, Buffer1));
//   printf("%d\n", FillHalfBufferWithZeros(1, Buffer2));
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBuffer(0, Buffer);
//   // FillHalfBufferWithZeros(0, Buffer);
//   // FillHalfBufferWithZeros(0, Buffer);
// }

// // void ModifyColcor (RGB) {
// //   stałe * R
// // }
