SDCCOPTS ?= --no-xinit-opt --iram-size 256 --xram-size 2048 --code-size 29696 -D__SDCC__ --stack-auto

SRC=main.c sx1278.c m10.c m20.c rs41.c dfm09.c
#SRC = $(wildcard *.c)
OBJ=$(patsubst %.c,build/%.rel, $(SRC))
BSP=../MS51_BSP/MS51FC0AE_MS51XC0BE_MS51EB0AE_MS51EC0AE_MS51TC0AE_MS51PC0AE
INC=-I$(BSP)/Library/StdDriver/inc -I$(BSP)/Library/Device/Include

build/%.rel: %.c
	mkdir -p $(dir $@)
	sdcc $(SDCCOPTS) $(INC) -o build/ -c $<

all: main

main: $(OBJ)
	sdcc -o build/ $(SDCCOPTS) $^ StdDriver.lib
	cp build/$@.ihx build/$@.hex
	hex2bin build/$@.hex > /dev/nul

build/main.rel:	sx1278.h rs41.h main.h m20.h m10.h dfm09.h
build/rs41.rel:	rs41.h main.h
build/m10.rel:	m10.h m20.h main.h
build/m20.rel:	m20.h main.h
build/dfm09.rel:	dfm09.h main.h

clean:
	rm -f build/*
