#ifndef __M20_H__
#define __M20_H__
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "main.h"

#define M20_PACKET_LENGTH 140U

extern const Sonde __code m20;
void descramble(uint8_t *frame,int packetLength);
#endif