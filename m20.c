#include "m20.h"

void descramble(uint8_t *frame,int packetLength) {
  uint8_t tmp, topbit=0;
  for (int i=0;i<packetLength;i++) {
    tmp = frame[i] << 7;
    frame[i] ^= 0xFF ^ (topbit | frame[i] >> 1);
    topbit = tmp;
  }
}

uint16_t m10CrcStep(uint16_t c, uint8_t b) {
  int c1 = c & 0xFF;
  // B
  b = (b >> 1) | ((b & 1) << 7);
  b ^= (b >> 2) & 0xFF;
  // A1
  int t6 = (c & 1) ^ ((c >> 2) & 1) ^ ((c >> 4) & 1);
  int t7 = ((c >> 1) & 1) ^ ((c >> 3) & 1) ^ ((c >> 5) & 1);
  int t = (c & 0x3F) | (t6 << 6) | (t7 << 7);
  // A2
  int s = (c >> 7) & 0xFF;
  s ^= (s >> 2) & 0xFF;
  int c0 = b ^ t ^ s;
  return ((c1 << 8) | c0) & 0xFFFF;
}

bool checkCrc(uint8_t *frame) {
    uint16_t crc = frame[M20_PACKET_LENGTH/2-2] << 8 | frame[M20_PACKET_LENGTH/2-1];
    uint16_t crc_calc = 0;
    for (int i=0;i<M20_PACKET_LENGTH/2-2;i++)
        crc_calc = m10CrcStep(crc_calc,frame[i]);
    return crc == crc_calc;
}

static void decodeFrame(uint8_t *a) {
  uint8_t b0=a[0x12];
  uint16_t s2=(a[0x14]<<8)|a[0x13];
  uint8_t ym=0x7F & b0;
  uint8_t y=ym/12;
  uint8_t m=ym%12+1;
  
  itoaWithZeroes(y,serial,10,1);
  itoaWithZeroes(m,serial+1,10,2);
  serial[3]='-';
  itoaWithZeroes(s2&0x3+2,serial+4,10,1);
  serial[5]='-';
  itoaWithZeroes((s2>>(2+13))&0x1,serial+6,10,1);
  itoaWithZeroes((s2>>2)&0x1FFF,serial+7,10,4);
  serial[11]='\0';
  lat = ((uint32_t)a[0x1C+0] << 24 | (uint32_t)a[0x1C+1] << 16 | (uint32_t)a[0x1C+2] << 8 | a[0x1C+3])/1e6;
  lng = ((uint32_t)a[0x20+0] << 24 | (uint32_t)a[0x20+1] << 16 | (uint32_t)a[0x20+2] << 8 | a[0x20+3])/1e6;
  alt = ((uint32_t)a[8]<<16 | (uint32_t)a[9]<<8 | a[10])/1e2;
  printPos(serial,lat,lng,alt);
}


int processPacketM20(uint8_t *buf) {
  if (!manchesterDecode(buf,buf,M20_PACKET_LENGTH))
    return 0;
  descramble(buf,M20_PACKET_LENGTH/2);
  if (checkCrc(buf))
    decodeFrame(buf);
  else {
    UARTSendString("Errore CRC\n");
    return 0;
  }
  return M20_PACKET_LENGTH/2;
}


const Sonde __code m20={
  .name="M20",
  .bitRate=9600,
  .afcBandWidth= 50000,
  .bandWidth=12500,
  .packetLength=M20_PACKET_LENGTH,
  .preambleLength=0,
  .syncWordLen= 48,
  .syncWord={ 0x66, 0x66, 0x66, 0x66, 0xB3, 0x66 },
  .processPacket=processPacketM20
};
