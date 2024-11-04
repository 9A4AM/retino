#include <CRC32.h>
extern "C" {
#include "Nuvoton8051.h"
#include "Nuvoton8051PlatformSpecific.h"
}
#include "fw.h"

uint8_t buffer[512];
CRC32 crc;

void dump(uint8_t *buf, int len) {
  int i,j;
  for (i = 0; i < len; i += 32) {
    Serial.print(i, HEX);
    Serial.print(": ");
    for (j = 0; j < 32 && i + j < len; j++) {
      if (buf[i + j] < 0x10) Serial.print("0");
      Serial.print(buf[i + j], HEX);
      Serial.print(" ");
    }
    for (;j<32;j++)
      Serial.print("   ");
    Serial.print(" ");
    for (j = 0; j < 32 && i + j < len; j++)
      Serial.print((char)(isprint(buf[i + j]) ? buf[i + j] : '.'));
    Serial.println();
  }
}

void readCfg() {
  Nuvoton8051_EntryMode();
  uint8_t cfg[5];
  Nuvoton8051_ReadCFG(cfg);
  Serial.println("Reading CFG");
  dump(cfg, 5);
  Nuvoton8051_ExitMode();
}

int nBytesInBuffer, currentAddress;

void flush() {
  if (nBytesInBuffer == 0) return;
  Serial.println(currentAddress);
  uint32_t res = Nuvoton8051_ProgramFlash(currentAddress, nBytesInBuffer, buffer, 1);
  if (res != currentAddress + nBytesInBuffer) {
    Serial.print("Error verifying at byte: ");
    Serial.println(res);
    return;
  }
  currentAddress += sizeof buffer;
  nBytesInBuffer = 0;
}

void writeByte(uint8_t b) {
  crc.add(b);
  buffer[nBytesInBuffer++] = b;
  if (nBytesInBuffer == sizeof buffer)
    flush();
}

int findLFS(uint8_t b) {
  for (int i = 0; i < sizeof lfs; i++)
    if (lfs[i] == b) return i;
  return -1;
}

void doWrite(uint8_t img[], uint16_t size, uint16_t address, uint32_t crcImg) {
  int i = 0, k;
  uint8_t b;

  crc.reset();
  nBytesInBuffer = 0;
  currentAddress = address;
  while (true) {
    b = pgm_read_byte(img + i);
    k = findLFS(b);
    if (k >= 0) {
      if (k == sizeof lfs - 1) {
        i++;
        b = pgm_read_byte(img + i);
        writeByte(b);
      } else {
        writeByte(digrams[k].a);
        writeByte(digrams[k].b);
      }
    } else
      writeByte(b);

    if (++i >= size) break;
  }
  flush();
  uint32_t crcCalc=crc.calc();
  if (crcImg != crcCalc) {
    Serial.print("CRC: ");
    Serial.print(crcImg, HEX);
    Serial.print("/");
    Serial.println(crcCalc, HEX);
    Serial.println("WRONG CRC !!!");
  }
}

void eraseFlash() {
  Serial.println("Erasing flash");
  Nuvoton8051_EntryMode();
  Nuvoton8051_MassErase();
  delay(200);
  Nuvoton8051_ExitMode();
}

void lock() {
  Nuvoton8051_EntryMode();
  Serial.println("locking: writing CFG");
  uint8_t cfg[] = { 0xFD, 0xFC, 0xFF, 0xFF, 0x5F };  //3kB LDROM size
  Nuvoton8051_ProgramCFG(cfg);
  Nuvoton8051_ExitMode();
}

void writeFlash() {
  Nuvoton8051_EntryMode();
  Serial.println("writing CFG");
  uint8_t cfg[] = { 0x6F, 0xFC, 0xFF, 0xFF, 0xFF };  //3kB LDROM size
  Nuvoton8051_ProgramCFG(cfg);

  doWrite(firmware, sizeof firmware, 0, firmware_CRC);
  doWrite(bootloader, sizeof bootloader, (32 - 3) * 1024U, bootloader_CRC);  //3kB LDROM size must be configured
  Serial.println("\ndone");
  Nuvoton8051_ExitMode();
}

void readFlash() {
  Nuvoton8051_EntryMode();
  for (unsigned i = 0; i < 32 * 1024U; i += sizeof buffer) {
    Serial.print("======= ");
    Serial.println(i, HEX);
    Nuvoton8051_ReadFlash(i, sizeof buffer, buffer);
    dump(buffer, sizeof buffer);
  }
  Nuvoton8051_ExitMode();
}

void getInfo() {
  Nuvoton8051_EntryMode();
  Serial.print("CID: ");
  uint8_t cid = Nuvoton8051_ReadCID();
  Serial.println(cid, HEX);

  Serial.print("DID: ");
  uint16_t did = Nuvoton8051_ReadDID();
  Serial.println(did, HEX);

  Serial.print("PID: ");
  uint16_t pid = Nuvoton8051_ReadPID();
  Serial.println(pid, HEX);
  Nuvoton8051_ExitMode();
}

void setup() {
  Serial.begin(115200);

  /*for (int i=0;i<10000;i++) crc.add(i);
  Serial.print("CRC ");
  Serial.println(crc.calc(),HEX);
  crc.reset();*/

  pinMode(LED_BUILTIN, OUTPUT);

  Nuvoton8051_Init();

  getInfo();
  readCfg();

  Serial.println("*** Use the 'w' command at your own risk! ***\nThe device firmware will be overwritten and lost forever!");
}

void loop() {
  String s = Serial.readStringUntil('\n');
  switch (tolower(s[0])) {
    case 'c':
      readCfg();
      break;
    case 'e':
      eraseFlash();
      break;
    case 'i':
      getInfo();
      break;
    case 'l':
      lock();
      break;
    case 'r':
      readFlash();
      break;
    case 'w':
      eraseFlash();
      writeFlash();
      break;
  }
}
