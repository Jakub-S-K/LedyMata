#include <stdint.h>

typedef struct {
  uint8_t Green;
  uint8_t Red;
  uint8_t Blue;
} COLOR;

// mnożniki
static COLOR gTemperatures[] = {{255, 255 255}};

LED_CONFIG *gConfigs = NULL;
uint8_t ConfigNumber = 0;

typedef struct {
  uint8_t LedCount;
  COLOR Color;
  uint8_t TemperatureIndex;
} LED_SECTION;

typedef struct {
  LED_SECTION *Sections;
  uint8_t SectionsSize;
  uint16_t BufferIndex;
  uint16_t BufferSize;
  uint8_t LedIndex;
  uint8_t SectionIndex;
} LED_CONFIG;

void InitializeConfigs(uint8_t ) {

}

void InitializeConfig(uint8_t ConfigIndex) {
  if (gConfigs == NULL) {
    malloc();
  }
  gConfigs[ConfigIndex].Sections = malloc(sizeof(LED_SECTION) * gConfigs[ConfigIndex].SectionSize);
  memset(gConfigs[ConfigIndex].Sections, 0, sizeof(LED_SECTION) * gConfigs[ConfigIndex].SectionSize);
  gConfigs[ConfigIndex].SectionsSize = gConfigs[ConfigIndex].SectionSize;
  gConfigs[ConfigIndex].BufferIndex = 0;
  gConfigs[ConfigIndex].BufferSize = gConfigs[ConfigIndex].BufferSize;
  gConfigs[ConfigIndex].LedIndex = 0;
  gConfigs[ConfigIndex].SectionIndex = 0;
  ConfigNumber++;
}

#define BRIGHTNESS 20

uint8_t GetPwmValueFromColor(uint8_t ConfigIndex) {
  static PwmValue[2] = {30, 60};
  uint8_t ColorOffset = (gConfigs[ConfigIndex].BufferIndex % 24) / 8;
  uint8_t TemperatureValue = (*(uint8_t*)(&(gTemperatures[gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].TemperatureIndex])) + ColorOffset) / 255;
  uint8_t ColorByte = *(((uint8_t*)&gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].Color) + ColorOffset);
  ColorByte = ColorByte * TemperatureValue * BRIGHTNESS;
  uint8_t ColorBitOffset = 7 - (gConfigs[ConfigIndex].BufferIndex % 8);
  return PwmValue[(ColorByte >> ColorBitOffset) & 1];
}

uint8_t FillHalfBuffer(uint8_t ConfigIndex, uint8_t *Buffer) {
  uint16_t BufferFilled = 0;
  uint16_t BufferToBeFilled = gConfigs[ConfigIndex].BufferSize / 2;
  uint16_t BufferSizeIteration = gConfigs[ConfigIndex].BufferIndex;

  if (gConfigs[ConfigIndex].BufferIndex != BufferToBeFilled && gConfigs[ConfigIndex].BufferIndex != 0) {
    return 2;
  }

  for (;gConfigs[ConfigIndex].SectionIndex < gConfigs[ConfigIndex].SectionsSize; gConfigs[ConfigIndex].SectionIndex++) {
    for (;gConfigs[ConfigIndex].LedIndex < gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].LedCount; gConfigs[ConfigIndex].LedIndex++) {
      if (BufferFilled + 24 > BufferToBeFilled) {
        BufferSizeIteration += BufferToBeFilled - BufferFilled;
      } else {
        BufferSizeIteration += 24;
      }
      for (;gConfigs[ConfigIndex].BufferIndex < BufferSizeIteration; gConfigs[ConfigIndex].BufferIndex++, BufferFilled++) {
        Buffer[gConfigs[ConfigIndex].BufferIndex] = GetPwmValueFromColor(ConfigIndex);
      }
    }
  }
  gConfigs[ConfigIndex].BufferIndex %= gConfigs[ConfigIndex].BufferSize;

  return BufferFilled != BufferToBeFilled;
}

// uint8_t FilHalfBufferWithZeros(uint8_t ConfigIndex, uint8_t *Buffer) {
  
// }

void PrepareBufferForTransaction(uint8_t ConfigIndex, uint8_t *Buffer) {
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

int main() {
  
}

// void ModifyColcor (RGB) {
//   stałe * R
// }
