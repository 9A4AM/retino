extern "C" {
#include "Nuvoton8051.h"
#include "Nuvoton8051PlatformSpecific.h"
}
#include "fw.h"
#include "bootloader.h"

uint8_t buffer[512];

void dump(uint8_t *buf, int len) {
  for (int i = 0; i < len; i += 32) {
    Serial.print(i, HEX);
    Serial.print(": ");
    for (int j = 0; j < 32 && i + j < len; j++) {
      if (buf[i + j] < 0x10) Serial.print("0");
      Serial.print(buf[i + j], HEX);
      Serial.print(" ");
    }
    Serial.print(" ");
    for (int j = 0; j < 32 && i + j < len; j++) {
      Serial.print((char)(isprint(buf[i + j]) ? buf[i + j] : '.'));
    }
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

bool doWrite(uint8_t img[], uint16_t size, uint16_t address) {
  Serial.print("writing ");
  Serial.print(size);
  Serial.println(" bytes");

  for (int i = 0; i < size; i += sizeof buffer) {
    Serial.println(i);
    int length = min(sizeof buffer, size - i);
    memcpy_P(buffer, img + i, length);
    uint32_t res = Nuvoton8051_ProgramFlash(address + i, length, buffer, 1);
    if (res != address + i + length) {
      Serial.print("Error verifying at byte: ");
      Serial.println(res);
      return false;
    }
  }
  return true;
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
  uint8_t cfg[] = { 0x6D, 0xFC, 0xFF, 0xFF, 0xFF };  //3kB LDROM size
  Nuvoton8051_ProgramCFG(cfg);  
  Nuvoton8051_ExitMode();
}

void writeFlash() {
  Nuvoton8051_EntryMode();
  Serial.println("writing CFG");
  uint8_t cfg[] = { 0x6F, 0xFC, 0xFF, 0xFF, 0xFF };  //3kB LDROM size
  Nuvoton8051_ProgramCFG(cfg);

  if (doWrite(firmware, sizeof firmware, 0))
    doWrite(bootloader, sizeof bootloader, (32 - 3) * 1024U);  //3kB LDROM size must be configured
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
