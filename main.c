//!make
//********!"c:\Program Files (x86)\Nuvoton Tools\NuLink Command Tool\NuLink_8051OT.exe" -w APROM build/main.bin
#include <numicro_8051.h>
#include <stdbool.h>
#include <spi.h>
#include <math.h>
#include <ctype.h>
#include "sx1278.h"
#include "rs41.h"
#include "m20.h"
#include "m10.h"
#include "dfm09.h"
#include "main.h"

/* TEST
RS41 405.950
M10 402.002
M20 402.002
DFM09 402.871
DFM17 403.250*/

typedef enum _SondeType { RS41, M10, M20, DFM09, DFM17 } SondeType;
const Sonde *sondes[]={&rs41,&m10,&m20,&dfm09,&dfm09};
SondeType sondeType=RS41;
uint32_t freq=405950000UL;
uint8_t __xdata buf[520];
uint16_t nCurByte=0;
char __xdata s[25], serial[12];
float lat,lng,alt;
uint64_t tLastPacket=0;
int packetLength;
volatile uint64_t _millis=0;
int volatile nRx=0;
char volatile __xdata rxBuff[20];
bool volatile messageReceived=false, messageRead=true;

void itoaWithZeroes(uint32_t val,char *dst,int base,int digits) {
  static const char __code tab[]="0123456789ABCDEF";
  for (int i=digits-1;i>=0;i--) {
    dst[i]=tab[val%base];
    val/=base;
  }
  dst[digits]='\0';
}

void dumpPacket(uint8_t buf[],int length) {
  for (int i=0;i<length;i++) {
    itoaWithZeroes(buf[i],s,16,2);
    UARTSendString(s);
  }
  UARTSendString("\n");
}

void printPos(char *serial,float lat,float lng,float alt) {
  //TODO: aggiungere RSSI
  UARTSendString("D");
  UARTSendString("Ser:");
  UARTSendString(serial);
  UARTSendString(",Lat:");
  __itoa(lat,s,10);
  UARTSendString(s);
  UARTSendString(".");
  itoaWithZeroes((lat-(int)lat)*1000000UL,s,10,6);
  UARTSendString(s);
  UARTSendString(",Lng:");
  __itoa(lng,s,10);
  UARTSendString(s);
  UARTSendString(".");
  itoaWithZeroes((lng-(int)lng)*1000000UL,s,10,6);
  UARTSendString(s);
  UARTSendString(",Alt:");
  __itoa(alt,s,10);
  UARTSendString(s);
  UARTSendString(".");
  itoaWithZeroes((alt-(int)alt)*10,s,10,1);
  UARTSendString(s);      
  UARTSendString("\n");
}

uint64_t millis(void) {
  DISABLE_GLOBAL_INTERRUPT;
  uint64_t res=_millis;
  ENABLE_GLOBAL_INTERRUPT;
  return res;
}

void Timer0_ISR (void) __interrupt (1) {
  PUSH_SFRS;
  SFRS = 0;
  TH0 = TH0TMP;	//reload Timer 0 counter
  TL0 = TL0TMP;
  clr_TCON_TF0;	//clear interrupt flag 
  _millis++;
  POP_SFRS;
}

void initSPI(void) {      
    P12_QUASI_MODE;                                  // (SS) Quasi mode
    P10_QUASI_MODE;                                  // P10 (SPCLK) Quasi mode
    P00_QUASI_MODE;                                  // P00 (MOSI) Quasi mode
    P01_QUASI_MODE;                                  // P01 (MISO) Quasi mode
    
    set_SPSR_DISMODF;                                // SS General purpose I/O ( No Mode Fault ) 
    clr_SPCR_SSOE;
   
    clr_SPCR_LSBFE;                                  // MSB first

    clr_SPCR_CPOL;                                   // The SPI clock is low in idle mode
    set_SPCR_CPHA;                                   // The data is sample on the second edge of SPI clock 
    
    set_SPCR_MSTR;                                   // SPI in Master mode 
    SPICLK_FSYS_DIV2;                                    // Select SPI clock 
    set_SPCR_SPIEN;                                  // Enable SPI function 
    clr_SPSR_SPIF;
}

void Serial_ISR(void) __interrupt (4) {
  //TODO: bufferizzazione
  PUSH_SFRS;
  if (RI) {
    uart0_receive_flag = 1;
    uart0_receive_data = SBUF;
    if (messageRead) {
      if (uart0_receive_data=='\n' || uart0_receive_data=='\r') {
	rxBuff[nRx]='\0';
	if (nRx>0) {
	  nRx=0;
	  messageReceived=true;
	}
      }
      else {
	if (nRx==sizeof buf-1) {
	  rxBuff[nRx]='\0';
	  nRx=0;
	  messageReceived=true;
	}
	else
	  rxBuff[nRx++]=uart0_receive_data;
      }
    }
    clr_SCON_RI;                                         // Clear RI (Receive Interrupt).
  }

  if (TI) {
      if (!PRINTFG)
	  TI = 0;
  }
  POP_SFRS;
}

void UARTSendString(const char *p) {
  DISABLE_UART0_INTERRUPT;
  while(*p)
    UART_Send_Data(UART0,*p++);
  ENABLE_UART0_INTERRUPT;
}

#ifdef __DEBUG__
void dumpRegisters(void) {
  for (int j=0;j<16;j++) {
    for (int i=0;i<7;i++) {
      sprintf(s,"%02X:%02X ",i*16+j,readRegister(i*16+j));
      UARTSendString(s);
    }
    UARTSendString("\n");
  }
}
#endif

void dump(uint8_t buf[], int size) {
  UARTSendString("\n(");
  for (int i = 0; i < size; i++) {
    //~ sprintf(s,"0x%02X%,", buf[i]);
    //~ UARTSendString(s);
    UARTSendString("0x");
    itoaWithZeroes(buf[i],s,16,2);
    UARTSendString(s);
    UARTSendString(",");
    if (i % 16 == 15)
      UARTSendString("\n");
  }
  //if (size % 16 != 0) UARTSendString("\n");
  UARTSendString(")\n");
}

uint8_t calcMantExp(uint16_t bw) {
  uint8_t exp = 1;
  bw = SX127X_CRYSTAL_FREQ / bw / 8;
  while (bw>31) { exp++; bw/=2.0; }
  uint8_t mant = bw<17?0 : bw<21? 1:2;
  return (mant<<3)|exp;
}

uint8_t flipByte(uint8_t b) {
  static const uint8_t __code rev[]={
    0x0, 0X8, 0X4, 0XC, 0X2, 0XA, 0X6, 0XE,
    0X1, 0X9, 0X5, 0XD, 0X3, 0XB, 0X7, 0XF
  };
  return rev[b>>4]|(rev[b&0x0F]<<4);
}

bool manchesterDecode(uint8_t* data, uint8_t *out, int len) {
  uint8_t t;
  uint16_t w;

  for (int i = 0; i < len / 2; i++) {
    w = (data[2 * i] << 8) | data[2 * i + 1];
    out[i] = 0;
    for (int j = 0; j < 8; j++) {
      out[i] <<= 1;
      t = (w >> 8) & 0b11000000;
      if (t == 0b01000000)
        out[i] |= 1;
      else if (t != 0b10000000) {
        UARTSendString("Err manchester\n");
        return false;
      }
      w <<= 2;
    }
  }
  return true;
}

void initRadio(void) {
  uint8_t mode;

  //reset SX1278
  P05=1;
  delay(10);
  P05=0;
  delay(1);
  P05=1;
  delay(5);

  writeRegister(RegOcp,0x3B);	//max current //TODO:

  writeRegister(RegOpMode,SLEEP_MODE);
  writeRegister(RegOpMode,SLEEP_MODE);
  writeRegister(RegOpMode,STANDBY_MODE);
  delay(100);

  mode=readRegister(RegOpMode);
  if (mode!=STANDBY_MODE) {
    UARTSendString("Errore, non in standby: ");
    DISABLE_UART0_INTERRUPT;
    UART_Send_Data(UART0,'0'+mode);
    UART_Send_Data(UART0,'\n');    
    ENABLE_UART0_INTERRUPT;
  }

  uint16_t bps=sondes[sondeType]->bitRate,
    bitRate = (SX127X_CRYSTAL_FREQ * 1.0) / bps,
    fracRate = (SX127X_CRYSTAL_FREQ * 16.0) / bps - bitRate * 16 + 0.5;
  writeRegister(RegBitRateMsb,bitRate>>8);
  writeRegister(RegBitRateLsb,bitRate);
  writeRegister(RegBitRateFrac, fracRate);
  
  writeRegister(RegAfcBw,calcMantExp(sondes[sondeType]->afcBandWidth));
  writeRegister(RegRxBw,calcMantExp(sondes[sondeType]->bandWidth));
  
  writeRegister(RegRxConfig,1<<4|1<<3|6);	//AFC, AGC, Rx Trigger AGC
  
  writeRegister(RegSyncConfig,1<<6|1<<4|(sondes[sondeType]->syncWordLen/8-1));		//autorestart w/o PLL wait, sync on, sync on, n bytes sync word
  if (sondes[sondeType]->preambleLength>0) {
    writeRegister(RegPreambleDetect,1u<<7|1u<<5|((sondes[sondeType]->preambleLength/8)-1));	//enabled, 2 bytes, 0 errors
    writeRegister(RegDioMapping2,1);		//Preambe detect enabled
  }
  else {
    writeRegister(RegPreambleDetect,0);		//disabled
    writeRegister(RegDioMapping2,0);		//Preamble detect disabled
  }

  //writeRegisters(RegSyncValue1,syncWord,sizeof syncWord);
  for (int i=0;i<sondes[sondeType]->syncWordLen/8;i++)
    writeRegister(RegSyncValue1+i,sondes[sondeType]->syncWord[i]);
  writeRegister(RegPacketConfig1,0x08);				//fixed length, no DC-free,no CRC,no filtering
  writeRegister(RegPacketConfig2,1u<<6|(packetLength>>8)&7); //packet mode,payload length msb
  writeRegister(RegPayloadLength,packetLength);	//payload length lsb

  writeRegister(RegDioMapping1,0<<6|0<<4);		//DIO0:Payload ready, DIO1:Fifo level
  
  writeRegister(RegFifoThresh,48);
  writeRegister(RegTcxo,0<<4);	//no TCXO

  //~ writeRegister(RegLna,0b110u<<5); //-48dB
  
  //~ writeRegister(RegOsc,1<<3);	//OSC calibration
  //~ while (readRegister(RegOsc)&1<<3) ;

  delay(1);

  uint32_t f = (freq/1000) * (1UL<<11);
  f /=  (SX127X_CRYSTAL_FREQ/1000)/(1UL<<8);
  writeRegister(RegFreqMsb, f>>16);
  writeRegister(RegFreqMid, f>>8);
  writeRegister(RegFreqLsb, f);

  writeRegister(RegOpMode,RX_MODE);
  delay(1);

  writeRegister(RegIrqFlags1,0xFF);
  writeRegister(RegIrqFlags2,0xFF);
    
  delay(1);

  //dumpRegisters();
}

void initGPIO(void) {
  P06_PUSHPULL_MODE;		//UART0 TX
  P07_INPUT_MODE;		//UART0 RX
  P13_INPUT_MODE;		//RXTX sx1278
  P15_QUASI_MODE;		//switch vcont1
  P17_QUASI_MODE;		//switch vcont2
  P04_INPUT_MODE;		//DIO1
  P03_INPUT_MODE;		//DIO2
  P05_QUASI_MODE;		//NRESET
  P11_PUSHPULL_MODE;		//LED
  P14_QUASI_MODE;		//DTR

  P11=0;
}


bool startsWith(const char *s, const char *prefix) {
  while (*prefix!=0)
    if (toupper(*s++)!=toupper(*prefix++)) return false;
  return true;
}

void printSettings(SondeType sondeType, uint32_t freq) {
  UARTSendString("#");
  UARTSendString(sondes[sondeType]->name);
  UARTSendString("@");
  __ultoa(freq,s,10);
  UARTSendString(s);
  UARTSendString("\n");
}

void selectSonde(char *buf) {
  SondeType t;
  uint32_t f=0;
  
  if (startsWith(buf,"RS41"))
    t=RS41;
  else if (startsWith(buf,"M10"))
    t=M10;
  else if (startsWith(buf,"M20"))
    t=M20;
  else if (startsWith(buf,"DFM09"))
    t=DFM09;
  else {
    UARTSendString("?\n");
    return;
  }
  
  for (int i=6;i<12;i++) {
    if (buf[i]<'0' || buf[i]>'9') {
      UARTSendString("?\n");
      return;
    }
    f*=10;
    f+=buf[i]-'0';
  }
  if (f<400000UL || f>=406000UL) {
    UARTSendString("?\n");
    return;
  }
  
  freq=f*1000UL;
  sondeType=t;
  lat=lng=alt=0;
  *serial='\0';
  
  packetLength=sondes[sondeType]->packetLength;

  initRadio();
  
  printSettings(sondeType,freq);
}

void rebootFromLDROM(void) {
  Software_Reset(BOOT_LDROM);
  while (1) ;
}

void main(void) {
  MODIFY_HIRC(HIRC_24);
  
  initGPIO();
  
  Timer0_AutoReload_Interrupt_Initial(24,1000);
    
  UART_Open(F_CPU,UART0_Timer3,115200);
  ENABLE_UART0_INTERRUPT;
  ENABLE_GLOBAL_INTERRUPT;

  //UARTSendString("\x1B[2J\x1B[HVia!\n");
  printSettings(sondeType,freq);

  packetLength=sondes[sondeType]->packetLength;
  
  //switch antenna to rx circuitry (hopefully)
  P15=0;
  P17=1;

  initSPI();
  initRadio();
  nCurByte=0;
  while (1) {
    /*uint8_t flags1=readRegister(RegIrqFlags1),
      flags2=readRegister(RegIrqFlags2);

    if (flags1&Flags1SyncAddressMatch) {
      writeRegister(RegIrqFlags1,Flags1SyncAddressMatch);
      UARTSendString("SYNC---------------\n");
      //nCurByte=0;
    }*/
    
    if (tLastPacket!=0 && millis()-tLastPacket>100) {
      P11=0;
      tLastPacket=0;
    }

    if (P04) {	//DIO0: payload ready
      while (nCurByte<packetLength) {
	buf[nCurByte]=readRegister(RegFIFO);
	nCurByte++;
      }
      //UARTSendString("PKT\n");
      P11=1;
      tLastPacket=millis();
      int nBytes=sondes[sondeType]->processPacket(buf);
      if (nBytes>0) {
	UARTSendString("P");
	dumpPacket(buf,nBytes);
      }
      nCurByte=0;
    }
    if (P03) {	//DIO1: fifo level
      for (int i=0;i<48 && nCurByte<packetLength;i++,nCurByte++)
	buf[nCurByte]=readRegister(RegFIFO);
    }
    if (messageReceived) {
      switch (rxBuff[0]) {
	case 0:
	case '@':
	  UARTSendString("reboot\n");
	  delay(300);
	  rebootFromLDROM();
	  break;
	case '?':
	  printSettings(sondeType,freq);
	  break;
	case '0':
	  P15=0;
	  P17=1;
	  UARTSendString("P15=0 P17=1\n");
	  break;
	case '1':
	  P15=1;
	  P17=0;
	  UARTSendString("P15=1 P17=0\n");
	  break;
	default:
	  selectSonde(rxBuff);
	  break;
      }
      messageReceived=false;
      messageRead=true;
    }
  }
} 