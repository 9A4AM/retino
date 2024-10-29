#include "dfm09.h"
//TODO: printPos
#define MAXLEN 13
enum datType { SEQ = 0,
               TIME = 1,
               LAT = 2,
               LON = 3,
               ALT = 4,
               DATE = 8 };

static void deinterleave(uint8_t* in, uint8_t* out, int len) {
  static __xdata uint8_t b, dataIn[8 * MAXLEN], dataOut[8 * MAXLEN];
  int i, j;

  for (i = 0; i < len; i++) {  //spread the bits
    b = in[i];
    for (j = 0; j < 8; j++) {
      dataIn[8 * i + j] = b >> 7;
      b <<= 1;
    }
  }

  for (j = 0; j < 8; j++)
    for (i = 0; i < len; i++)
      dataOut[8 * i + j] = dataIn[len * j + i];

  for (i = 0; i < len; i++) {  //squeeze back the bits
    b = 0;
    for (j = 0; j < 8; j++) {
      b <<= 1;
      b |= dataOut[8 * i + j];
    }
    out[i] = b;
  }
}

static int parity(uint8_t x) {
  int ret;

  for (ret = 0; x; ret++)
    x &= x - 1;

  return ret & 1;
}

static int hamming(uint8_t* data, int len) {
  static const uint8_t __code hamming_bitmasks[] = { 0xaa, 0x66, 0x1e, 0xff };
  int errpos, errcount = 0;
  int i, j;

  for (i = 0; i < len; i++) {
    errpos = 0;
    for (j = 0; j < (int)sizeof(hamming_bitmasks); j++)
      errpos += (1 << j) * parity(data[i] & hamming_bitmasks[j]);

    if (errpos > 7) return -1;

    if (errpos) {
      errcount++;
      data[i] ^= 1 << (8 - errpos);
    }
  }

  return errcount;
}

static void processConf(uint8_t type, uint8_t* data) {
  static int serialNumberConfType=-1;
  static uint64_t raw_serial=0;
  bool valid = false;
  uint32_t ch = (uint32_t)data[0] << 16 | (uint32_t)data[1] << 8 | data[2];

  for (int i = 0; i < 7 / 2; i++)
    if (data[i] != 0) {
      valid = true;
      break;
    }

  if (ch == 0) {
    serialNumberConfType = type + 1;
    return;
  }

  if (!valid) return;

  if (type == serialNumberConfType) {
    int serial_idx = 3 - (ch & 0xF);
    uint16_t serial_shard = ch >> 4;
    raw_serial &= ~((uint64_t)((1ull << 16) - 1) << (16u * serial_idx));
    raw_serial |= (uint64_t)serial_shard << (16u * serial_idx);
    if ((ch & 0x0F) == 0) {
      while (raw_serial && !(raw_serial & 0xFFFFu)) raw_serial >>= 16;
      __ultoa(raw_serial,serial,10);
      serial[sizeof serial - 1] = '\0';
      raw_serial = 0;
      /*UARTSendString("\t\tSerial: ");
      UARTSendString(serial);
      UARTSendString("\n");*/
    }
  }
}

static void processDat(uint8_t type, uint8_t* data) {
  //uint16_t sec;
  bool valid = false;

  for (int i = 0; i < 13 / 2; i++)
    if (data[i] != 0) {
      valid = true;
      break;
    }
  if (!valid) return;

  switch (type) {
    /*case TIME:
      sec = data[4] << 8 | data[5];
      UARTSendString("\tSec: ");
      __itoa(sec/1000,s,10);
      UARTSendString(s);
      UARTSendString("\n");
      break;*/
    case LAT:
      lat = ((uint32_t)data[0] << 24 | (uint32_t)data[1] << 16 | (uint32_t)data[2] << 8 | (uint32_t)data[3]) / 1e7;
      /*UARTSendString("\tlat: ");
      __itoa(lat,s,10);
      UARTSendString(s);
      UARTSendString(".");
      __ultoa((uint32_t)((lat-(int)lat)*1000000UL),s,10);
      UARTSendString(s);
      UARTSendString("\n");*/
      break;
    case LON:
      lng = ((uint32_t)data[0] << 24 | (uint32_t)data[1] << 16 | (uint32_t)data[2] << 8 | (uint32_t)data[3]) / 1e7;
      /*UARTSendString("\tlng: ");
      __itoa(lng,s,10);
      UARTSendString(s);
      UARTSendString(".");
      __ultoa((uint32_t)((lng-(int)lat)*1000000UL),s,10);
      UARTSendString(s);
      UARTSendString("\n");*/
      break;
    case ALT:
      alt = ((uint32_t)data[0] << 24 | (uint32_t)data[1] << 16 | (uint32_t)data[2] << 8 | (uint32_t)data[3]) / 1e2;
      /*UARTSendString("\talt: ");
      __itoa(alt,s,10);
      UARTSendString(s);
      UARTSendString(".");
      __itoa((uint32_t)((alt-(int)alt)*10),s,10);
      UARTSendString(s);
      UARTSendString("\n");*/
      break;
    default:
      valid=false;
  }
  if (valid) printPos(serial,lat,lng,alt);
}

int processPacketDFM09(uint8_t *buf) {
  uint8_t dat[13];
  
  if (!manchesterDecode(buf,buf,DFM09_PACKET_LENGTH))
    return 0;
  deinterleave(buf, dat,  7);
  int err = hamming(dat, 7);
  if (err < 0) {
    UARTSendString("\nECC conf\n");
    return 0;
  }
  else {
    uint8_t confType = dat[0] >> 4;
    //sprintf(s,"conf type: %1X, data: ", confType);
    //UARTSendString(s);
    for (int i = 0; i < 7 / 2; i++) dat[i] = dat[2 * i + 1] & 0xF0 | dat[2 * i + 2] >> 4;
    //~ sprintf(s,"\n%02X ",confType);
    //~ UARTSendString(s);
    //~ dump(dat, 7 / 2);
    processConf(confType, dat);
  }

  deinterleave(buf + 7, dat, 13);
  err = hamming(dat, 13);
  if (err < 0) {
    UARTSendString("\nECC DAT1\n");
    return 0;
  }
  else {
    uint8_t dat1Type = dat[12] >> 4;
    //Serial.printf("dat1 type: %1X, data: ", dat1Type);
    for (int i = 0; i < 13 / 2; i++) dat[i] = dat[2 * i] & 0xF0 | dat[2 * i + 1] >> 4;
    //dump(dat, 13 / 2);
    processDat(dat1Type, dat);
  }

  deinterleave(buf + 7 + 13, dat, 13);
  err = hamming(dat, 13);
  if (err < 0) {
    UARTSendString("\nECC DAT2\n");
    return 0;
  }
  else {
    uint8_t dat2Type = dat[12] >> 4;
    //Serial.printf("dat2 type: %1X, data: ", dat2Type);
    for (int i = 0; i < 13 / 2; i++) dat[i] = dat[2 * i] & 0xF0 | dat[2 * i + 1] >> 4;
    //dump(dat, 13 / 2);
    processDat(dat2Type, dat);
  }
  return DFM09_PACKET_LENGTH/2;
}

const Sonde __code dfm09={
  .name="DFM09",
  .bitRate=2500,
  .afcBandWidth= 50000,
  .bandWidth=11700,
  .packetLength= DFM09_PACKET_LENGTH,
  .preambleLength=0,
  .syncWordLen=32,
  .syncWord={ 0x9A, 0x99, 0x5A, 0x55 },
  .processPacket=processPacketDFM09
};
