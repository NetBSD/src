#PROG: strip
#source: compressed-1.s
#as: --64 --compress-debug-sections
#strip:
#readelf: -S --wide
#name: strip on compressed debug sections

There are 5 section headers, starting at offset 0x78:

Section Headers:
  \[Nr\] Name              Type            Address          Off    Size   ES Flg Lk Inf Al
  \[ 0\]                   NULL            0000000000000000 000000 000000 00      0   0  0
  \[ 1\] .text             PROGBITS        0000000000000000 000040 000015 00  AX  0   0 16
  \[ 2\] .data             PROGBITS        0000000000000000 000058 000000 00  WA  0   0  4
  \[ 3\] .bss              NOBITS          0000000000000000 000058 000000 00  WA  0   0  4
  \[ 4\] .shstrtab         STRTAB          0000000000000000 000058 00001c 00      0   0  1
Key to Flags:
#...
