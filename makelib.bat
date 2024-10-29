rem aggiungere static alla dichiarazione di hircmap0,hircmap1,offset,judge in sys.c (per modello large)
@echo off
rem set opts=--model-large --no-xinit-opt -c -I ..\..\Device\Include --stack-auto -I ..\inc -D__SDCC__
set opts=--no-xinit-opt -c -I ..\..\Device\Include --stack-auto -I ..\inc -D__SDCC__
set bsp=..\MS51_BSP\MS51FC0AE_MS51XC0BE_MS51EB0AE_MS51EC0AE_MS51TC0AE_MS51PC0AE
pushd %bsp%\Library\StdDriver\src
sdcc %opts% adc.c
sdcc %opts% bod.c
sdcc %opts% capture.c
sdcc %opts% common.c
sdcc %opts% delay.c
sdcc %opts% eeprom.c
sdcc %opts% eeprom_sprom.c
sdcc %opts% gpio.c
sdcc %opts% i2c.c
sdcc %opts% iap.c
sdcc %opts% iap_sprom.c
sdcc %opts% isr.c
rem sdcc %opts% putchar.c
sdcc %opts% pwm0.c
sdcc %opts% pwm123.c
rem sdcc %opts% sdcc_stdio.c
sdcc %opts% spi.c
sdcc %opts% sys.c
sdcc %opts% timer.c
sdcc %opts% uart.c
sdcc %opts% uart2.c
sdcc %opts% uart3.c
sdcc %opts% uart4.c
sdcc %opts% wdt.c
sdcc %opts% wkt.c
rem for %%f in (*.c) do sdcc %opts% %%f
sdar -rc StdDriver.lib adc.rel bod.rel capture.rel common.rel delay.rel eeprom.rel eeprom_sprom.rel gpio.rel i2c.rel iap.rel iap_sprom.rel isr.rel pwm0.rel pwm123.rel spi.rel sys.rel timer.rel uart.rel uart2.rel uart3.rel uart4.rel wdt.rel wkt.rel
del *.asm *.lst *.sym *.rel
popd
xcopy /y %bsp%\Library\StdDriver\src\*.lib .
