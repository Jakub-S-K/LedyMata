#include <stdint.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "dupaleds.h"

static COLOR gTemperatures[] = {{255, 255, 255}};

typedef struct
{
  uint16_t *Buffer;
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

#ifdef DEBUG
#define PRINT printf
#define BUFFER_DUMP BufferDump
#else
#define PRINT
#define BUFFER_DUMP
#endif

#ifdef DEBUG

void BufferDump(uint8_t ConfigIndex)
{
  PRINT("Buffer dump:\n");
  for (int i = 0; i < gConfigs[ConfigIndex].BufferSize / 24; i++)
  {
    PRINT("%1d [", i);
    for (int j = 0; j < 24; j++)
    {
      PRINT("%d:%1d ", j, gConfigs[ConfigIndex].Buffer[i * 24 + j] );
    }
    PRINT("\b]\n");
  }
  PRINT("\n");
}

#endif

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

uint8_t InitializeConfig(uint8_t ConfigIndex, uint8_t AmountOfSections, uint8_t *LedCounts, uint16_t *Buffer, uint8_t BufferSize)
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
  gConfigs[ConfigIndex].Buffer = Buffer;
  gConfigNumber++;
  return 0;
}

#define BRIGHTNESS 255

LED_SECTION *GetLedSection(uint8_t ConfigIndex, uint8_t SectionIndex)
{
  return &gConfigs[ConfigIndex].Sections[SectionIndex];
}

uint8_t GetPwmValueFromColor(uint8_t ConfigIndex)
{
#ifdef DEBUG
  static uint8_t PwmValue[2] = {0, 1};
#else
  static uint8_t PwmValue[2] = {30, 60};
#endif
  uint8_t ColorOffset = (gConfigs[ConfigIndex].BufferIndex % 24) / 8;
  uint8_t *temp = (uint8_t *)(&(gTemperatures[gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].TemperatureIndex]));
  uint8_t TemperatureValue = (*(uint8_t *)(temp + ColorOffset));
  PRINT("%p %p\n", &(gTemperatures[gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].TemperatureIndex]) + ColorOffset, gTemperatures);
  PRINT("%d\n", *((uint8_t*)(&(gTemperatures[gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].TemperatureIndex]))));
  uint32_t ColorByte = *(((uint8_t *)&gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].Color) + ColorOffset);
  PRINT("ColorByte %d\n", ColorByte);
  ColorByte = (ColorByte * (uint32_t)(TemperatureValue * BRIGHTNESS));
  ColorByte = ColorByte / ((uint32_t)(255 * 255));
  PRINT("Tak %d\n", (uint32_t)(TemperatureValue * BRIGHTNESS));
  PRINT("Temperature %d\n", (TemperatureValue ));
  uint8_t ColorBitOffset = 7 - (gConfigs[ConfigIndex].BufferIndex % 8);
  PRINT("ColorByte %d; ColorBitOffset %d; PwmValue %d;\n", ColorByte, ColorBitOffset, PwmValue[(ColorByte >> ColorBitOffset) & 1]);
  return PwmValue[(ColorByte >> ColorBitOffset) & 1];
}

uint8_t FillHalfBuffer(uint8_t ConfigIndex)
{
  uint16_t BufferFilled = 0;
  uint16_t BufferToBeFilled = gConfigs[ConfigIndex].BufferSize / 2;
  uint16_t BufferSizeIteration = gConfigs[ConfigIndex].BufferIndex;
  uint8_t ReturnValue = 0;

  if (gConfigs[ConfigIndex].BufferIndex != BufferToBeFilled && gConfigs[ConfigIndex].BufferIndex != 0)
  {
    return 2;
  }

  for (; gConfigs[ConfigIndex].SectionIndex < gConfigs[ConfigIndex].SectionsSize; gConfigs[ConfigIndex].SectionIndex++)
  {
    PRINT("Entered\n");
    PRINT("[%d] LedIndex %d; BufferIndex %d\n", gConfigs[ConfigIndex].SectionIndex, gConfigs[ConfigIndex].LedIndex, gConfigs[ConfigIndex].BufferIndex);
    for (; gConfigs[ConfigIndex].LedIndex < gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].LedCount; gConfigs[ConfigIndex].LedIndex++)
    {
      if (BufferFilled == BufferToBeFilled)
      {
        PRINT("Exiting\n");
        goto end;
      }
      if (BufferFilled + 24 > BufferToBeFilled)
      {
        BufferSizeIteration += BufferToBeFilled - BufferFilled;
      }
      else
      {
        BufferSizeIteration += 24;
      }
      PRINT("[%d.%d] BufferSizeIteration %d\n", gConfigs[ConfigIndex].SectionIndex, gConfigs[ConfigIndex].LedIndex, BufferSizeIteration);
      for (; gConfigs[ConfigIndex].BufferIndex < BufferSizeIteration; gConfigs[ConfigIndex].BufferIndex++, BufferFilled++)
      {
        PRINT("[%d.%d.%d]\n", gConfigs[ConfigIndex].SectionIndex, gConfigs[ConfigIndex].LedIndex, gConfigs[ConfigIndex].BufferIndex);
        gConfigs[ConfigIndex].Buffer[gConfigs[ConfigIndex].BufferIndex] = GetPwmValueFromColor(ConfigIndex);
      }
    }
    if (BufferFilled == BufferToBeFilled)
    {
      goto end;
    }

    gConfigs[ConfigIndex].LedIndex = 0;
  }
  PRINT("BufferIndex %d; BufferSize %d\n", gConfigs[ConfigIndex].BufferIndex, gConfigs[ConfigIndex].BufferSize);
  PRINT("%d, %d\n", BufferFilled, BufferToBeFilled);

  if (BufferFilled != BufferToBeFilled) {
    for (; BufferFilled < BufferToBeFilled; BufferFilled++, gConfigs[ConfigIndex].BufferIndex++) {
      gConfigs[ConfigIndex].Buffer[gConfigs[ConfigIndex].BufferIndex] = 0;      
    }

    ReturnValue = 1;
  }
  
end:
  gConfigs[ConfigIndex].BufferIndex %= gConfigs[ConfigIndex].BufferSize;
  BUFFER_DUMP(ConfigIndex);
  return ReturnValue;
}

uint8_t PrepareBufferForTransaction(uint8_t ConfigIndex)
{
  uint8_t ReturnValue = 0;

  gConfigs[ConfigIndex].LedIndex = 0;
  gConfigs[ConfigIndex].SectionIndex = 0;
  gConfigs[ConfigIndex].BufferIndex = 0;
  ReturnValue = FillHalfBuffer(ConfigIndex);
  ReturnValue = FillHalfBuffer(ConfigIndex);
  return ReturnValue;
}

#ifdef DEBUG

void CheckValues(uint8_t LedsAmount, COLOR* LedColors, uint16_t* Buffer) {
  uint8_t LedIndex;
  int8_t BitIndex;
  uint8_t Index;
  const uint8_t ColorTable[] = {0, 1};
  uint8_t ExpectedColor;
  uint8_t ActualColor;
  uint8_t ExpectedBufferIndex;
  uint8_t ActualBufferIndex;
  uint8_t *Colors = (uint8_t*)(LedColors);
  uint8_t Failed = 0;

  for (LedIndex = 0; LedIndex < LedsAmount; LedIndex++) {
    for (Index = 0; Index < 3; Index++) {
      for (BitIndex = 7; BitIndex >= 0; BitIndex--) {
        ExpectedBufferIndex = LedIndex * 3 + Index;
        ActualBufferIndex = (LedIndex * 3 * 8) + (Index * 8) + BitIndex;
        ExpectedColor = ColorTable[((Colors[ExpectedBufferIndex] >> (7 - BitIndex)) & 0x1)];
        ActualColor = Buffer[ActualBufferIndex];
        if (ExpectedColor != ActualColor) {
          Failed = 1;
          PRINT("Led[%d] Color[%d] Bit[%d] incorrect should be %d is %d Actual %d Expected %d\n", LedIndex, Index, BitIndex, ExpectedColor , ActualColor, ActualBufferIndex, ExpectedBufferIndex);
        }
      }
    }
  }
  assert(Failed == 0);
}

int main()
{
  uint16_t Buffer1[24 * 8];
  InitializeConfigs(1);
  uint8_t leds[] = {10, 12, 14, 16};
  InitializeConfig(0, 4, leds, Buffer1, 8 * 24);
  COLOR tak = {150, 160, 170};
  COLOR tak1 = {150, 160, 170};
  COLOR tak2 = {10, 20, 30};
  COLOR tak3 = {12, 18, 15};
  COLOR zero = {0, 0, 0};
  GetLedSection(0, 0)->Color = tak;
  GetLedSection(0, 1)->Color = tak1;
  GetLedSection(0, 2)->Color = tak2;
  GetLedSection(0, 3)->Color = tak3;
  PrepareBufferForTransaction(0);
  COLOR LedColors1[8] = {tak, tak, tak, tak, tak, tak, tak, tak};
  CheckValues (8, LedColors1, Buffer1);
  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR LedColors2[8] = {tak, tak, tak1, tak1, tak, tak, tak, tak};
  CheckValues (8, LedColors2, Buffer1);
  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR LedColors3[8] = {tak, tak, tak1, tak1, tak1, tak1, tak1, tak1};
  CheckValues (8, LedColors3, Buffer1);

  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR LedColors4[8] = {tak1, tak1, tak1, tak1, tak1, tak1, tak1, tak1};
  CheckValues (8, LedColors4, Buffer1);
  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR LedColors5[8] = {tak1, tak1, tak1, tak1, tak1, tak1, tak2, tak2};
  CheckValues (8, LedColors5, Buffer1);

  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR LedColors6[8] = {tak2, tak2, tak2, tak2, tak1, tak1, tak2, tak2};
  CheckValues (8, LedColors6, Buffer1);
  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR LedColors7[8] = {tak2, tak2, tak2, tak2, tak2, tak2, tak2, tak2};
  CheckValues (8, LedColors7, Buffer1);

  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR LedColors8[8] = {tak2, tak2, tak2, tak2, tak2, tak2, tak2, tak2};
  CheckValues (8, LedColors8, Buffer1);
  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR LedColors9[8] = {tak2, tak2, tak2, tak2, tak3, tak3, tak3, tak3};
  CheckValues (8, LedColors9, Buffer1);

  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR LedColors10[8] = {tak3, tak3, tak3, tak3, tak3, tak3, tak3, tak3};
  CheckValues (8, LedColors10, Buffer1);
  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR LedColors11[8] = {tak3, tak3, tak3, tak3, tak3, tak3, tak3, tak3};
  CheckValues (8, LedColors11, Buffer1);

  PRINT("End value 12: %d\n", FillHalfBuffer(0));
  COLOR LedColors12[8] = {tak3, tak3, tak3, tak3, tak3, tak3, tak3, tak3};
  CheckValues (8, LedColors12, Buffer1);
  PRINT("End value 13: %d\n", FillHalfBuffer(0));
  COLOR LedColors13[8] = {tak3, tak3, tak3, tak3, zero, zero, zero, zero};
  CheckValues (8, LedColors13, Buffer1);
  PRINT("End value 14: %d\n", FillHalfBuffer(0));
  COLOR LedColors14[8] = {zero, zero, zero, zero, zero, zero, zero, zero};
  CheckValues (8, LedColors14, Buffer1);
}

#endif

// void ModifyColcor (RGB) {
//   stałe * R
// }
