#include "m10.h"

static float m10_9f_lat(const M10Frame_9f* f) {
  int32_t lat =  (uint32_t)f->lat[0] << 24 | (uint32_t)f->lat[1] << 16 | (uint32_t)f->lat[2] << 8 | f->lat[3];
  return lat * 360.0 / ((uint64_t)1UL << 32);
}

static float m10_9f_lon(const M10Frame_9f* f) {
  int32_t lon = (uint32_t)f->lon[0] << 24 | (uint32_t)f->lon[1] << 16 | (uint32_t)f->lon[2] << 8 | f->lon[3];
  return lon * 360.0 / ((uint64_t)1UL << 32);
}

static float m10_9f_alt(const M10Frame_9f* f) {
  int32_t alt =  (uint32_t)f->alt[0] << 24 | (uint32_t)f->alt[1] << 16 | (uint32_t)f->alt[2] << 8 | f->alt[3];
  return alt / 1e3;
}

static void m10_9f_serial(char *dst, const M10Frame_9f *frame) {
  uint16_t serial_0 = ((uint16_t)frame->serial[2] >> 4) * 100UL + (frame->serial[2] & 0xF),
    serial_1 = frame->serial[0],
    serial_2 = frame->serial[3] | (uint16_t)frame->serial[4] << 8;

  itoaWithZeroes(serial_0,dst,10,3);
  itoaWithZeroes(serial_1,dst+4,10,1);
  itoaWithZeroes(serial_2>>13,dst+6,10,1);
  itoaWithZeroes(serial_2 & 0x1FFF,dst+7,10,4);
  dst[3]=dst[5]='-';
  dst[11]='\0';
}

static int m10_frame_correct(M10Frame_9f *frame) {
	const uint8_t *raw_frame = (uint8_t*)&frame->len;
	const uint8_t *crc_ptr = (uint8_t*)&frame->len + frame->len - 1;
	const uint16_t expected = crc_ptr[0] << 8 | crc_ptr[1];
	uint16_t crc;

	crc = 0;
	for (; raw_frame < crc_ptr; raw_frame++)
		crc = m10CrcStep(crc, *raw_frame);

	return (crc == expected) ? 0 : -1;
}

 int processPacketM10(uint8_t buf[]) {
  static M10Frame_9f __xdata frame;
  frame.sync_mark[0]=
  frame.sync_mark[1]=0x55;
  frame.sync_mark[2]=0x85;

  if (manchesterDecode(buf,&frame.len, M10_PACKET_LENGTH)) {
    for (int i=0;i<M10_PACKET_LENGTH/2;i++) ((uint8_t*)&frame.len)[i]^=0xFF;
    descramble((uint8_t*)&frame,sizeof(M10Frame));
    if (m10_frame_correct(&frame)==0) {
      m10_9f_serial(serial,&frame);
      lat=m10_9f_lat(&frame);
      lng=m10_9f_lon(&frame);
      alt=m10_9f_alt(&frame);
      printPos(serial,lat,lng,alt);
      return M10_PACKET_LENGTH/2;
    }
    else
      UARTSendString("processPacketM10: Errore CRC\n");
  }
  return 0;
}

const Sonde __code m10={
  .name="M10",
  .bitRate=9615,
  .afcBandWidth= 50000,
  .bandWidth=12500,
  .packetLength=M10_PACKET_LENGTH,
  .preambleLength=0,
  .syncWordLen=48,
  .syncWord={ 0x66, 0x66, 0x66, 0x66, 0xB3, 0x66 },
  .processPacket=processPacketM10
};