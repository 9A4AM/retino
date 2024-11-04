# Digraph compression for firmware and bootloader images
import os, sys, urllib.request
from crc import Calculator, Crc32

N = 64
chr = {}
dig = {}
calculator = Calculator(Crc32.CRC32)


def doAnalyze(filename):
    global lfs, mfd, chr, dig

    with open(filename, "rb") as file:
        data = file.read()

    for i in data:
        if i in chr:
            chr[i] += 1
        else:
            chr[i] = 1

    for i in range(len(data) - 1):
        x = data[i], data[i + 1]
        if x in dig:
            dig[x] += 1
        else:
            dig[x] = 1

    lfs = sorted(chr.items(), key=lambda x: x[1])[:N]
    lfs = [i[0] for i in lfs]
    mfd = sorted(dig.items(), key=lambda x: x[1], reverse=True)[: N - 1]
    mfd = [i[0] for i in mfd]


def analyze(filenames):
    global lfs, mfd

    for filename in filenames:
        doAnalyze(filename)

    print(
        """#include <stdint.h>

typedef struct {uint8_t a,b;} Digram;
"""
    )
    print(f"//{N} less frequent symbols:")
    print("uint8_t lfs[]={\n\t", end="")
    for i,s in enumerate(lfs):
        print(f"0x{s:02X},", end="\n\t" if i%16==15 else "")
    print("\n};\n")

    print(f"//{N-1} most frequent digrams:")
    print("Digram digrams[]={\n\t", end="")
    for i,d in enumerate(mfd):
        print(f"{{0x{d[0]:02X},0x{d[1]:02X}}},", end="\n\t" if i%8==7 else "")
    print("\n};\n")


def compress(filename, varname):
    global lfs, mfd

    with open(filename, "rb") as file:
        data = file.read()

    print(f"uint32_t {varname}_CRC=0x{calculator.checksum(bytearray(data)):08X}UL;\n")

    i = 0
    nBytes = 0
    bytes = []
    while True:
        if data[i] in lfs:
            bytes.append(lfs[N - 1])
            bytes.append(data[i])
            nBytes += 2
        else:
            if i < len(data) - 1:
                try:
                    k = mfd.index((data[i], data[i + 1]))
                    bytes.append(lfs[k])
                    i += 1
                    nBytes += 1
                except ValueError:
                    bytes.append(data[i])
                    nBytes += 1
            else:
                bytes.append(data[i])
                nBytes += 1
        i += 1
        if i >= len(data):
            break

    print(
        f"//size uncompressed: {len(data)}, compressed: {nBytes}, ratio {100*nBytes//len(data)}%"
    )

    print(f"const uint8_t {varname}[] PROGMEM={{\n\t", end="")
    for i, b in enumerate(bytes):
        print(f"0x{b:02X},", end="\n\t" if i % 16 == 15 else "")
    print("\n};")
    # test decompressione
    i = 0
    data1 = bytearray([])
    while True:
        try:
            k = lfs.index(bytes[i])
            if k == N - 1:
                i += 1
                data1.append(bytes[i])
            else:
                data1.append(mfd[k][0])
                data1.append(mfd[k][1])
        except ValueError:
            data1.append(bytes[i])
        i += 1
        if i >= len(bytes):
            break

    print(f"//decompression test {'OK' if data==data1 else 'KO'}\n")

    if len(data) != len(data1):
        print(f"//{len(data)} vs {len(data1)} bytes")


# TODO: download ISP_UART0.bin
url = "https://github.com/OpenNuvoton/MS51_BSP/raw/refs/heads/master/MS51FC0AE_MS51XC0BE_MS51EB0AE_MS51EC0AE_MS51TC0AE_MS51PC0AE/SampleCode/ISP/ISP_UART0/ExcutableBin/ISP_UART0.bin"
if not os.path.isfile("build/ISP_UART0.bin"):
    urllib.request.urlretrieve(url, "build/ISP_UART0.bin")
analyze(["build/main.bin", "build/ISP_UART0.bin"])
compress("build/main.bin", "firmware")
compress("build/ISP_UART0.bin", "bootloader")
