#include <Arduino.h>
#include "Nuvoton8051PlatformSpecific.h"

#define RST 2
#define CLK 3
#define DAT 4

void Nuvoton8051_GpioInit() {
  // init GPIO ports for RST, CLK and DAT pin to OUTPUT mode
  pinMode(RST, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(DAT, OUTPUT);
}


void Nuvoton8051_DelayUs(uint32_t periodUs) {
  // while function is named DelayUs is better to delay aprox 20 - 50 us instead.
  // remember to multiple wait time with parameter.
  delayMicroseconds(periodUs * 30);
}

void Nuvoton8051_GpioSetRstState(uint32_t level) {
  digitalWrite(RST, level ? HIGH : LOW);
}

void Nuvoton8051_GpioSetClkState(uint32_t level) {
  digitalWrite(CLK, level ? HIGH : LOW);
}

void Nuvoton8051_GpioSetDatState(uint32_t level) {
  digitalWrite(DAT, level ? HIGH : LOW);
}

uint32_t Nuvoton8051_GpioGetDatState() {
  return digitalRead(DAT)==HIGH;
}

void Nuvoton8051_GpioSetDatAsInput() {
  pinMode(DAT, INPUT);
}

void Nuvoton8051_GpioSetDatAsOutput() {
  pinMode(DAT, OUTPUT);
}
