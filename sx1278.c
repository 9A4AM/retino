#include <numicro_8051.h>
#include <spi.h>
#include "sx1278.h"

#undef SS_PIN
#define  SS_PIN             P12

uint8_t readRegister(uint8_t r) {
  SS_PIN=0;
  Spi_Write_Byte(r);
  uint8_t res = Spi_Read_Byte(0);
  SS_PIN=1;
	return res;
}

/*void writeRegisters(uint8_t r, uint8_t *values,int length) {
  SS_PIN=0;
  Spi_Write_Byte(0x80|r);
  while (length--)
    Spi_Write_Byte(*values++);
  SS_PIN=1;
}*/

void writeRegister(uint8_t r, uint8_t value) {
  SS_PIN=0;
  Spi_Write_Byte(0x80|r);
  Spi_Write_Byte(value);
  SS_PIN=1;
}