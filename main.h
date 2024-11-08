//!make
#ifndef __MAIN_H__
#define __MAIN_H__
//#define __DEBUG__

#include <stdlib.h>

#define SYNCWORD_SIZE 8
#define F_CPU 24E6
#define delay(ms) Timer1_Delay(F_CPU,ms,1000)
#define SX127X_CRYSTAL_FREQ 32000000UL

typedef struct Sonde_s {
  const char *name;
  unsigned bitRate, afcBandWidth, bandWidth;
  int packetLength, preambleLength, syncWordLen;
  uint8_t syncWord[SYNCWORD_SIZE];
  int (*processPacket)(uint8_t buf[]);
} Sonde;

extern char __xdata s[25], serial[12];
extern float lat,lng,alt;

void UARTSendString(const char *p);
uint8_t flipByte(uint8_t b);
bool manchesterDecode(uint8_t* data, uint8_t *out, int len);
void dump(uint8_t buf[], int size);
void itoaWithZeroes(uint32_t val,char *dst,int base,int digits);
void printPos(void);
void dumpPacket(uint8_t buf[],int length);
#endif