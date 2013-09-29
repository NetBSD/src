#PROG: strip
#source: compressed-1.s
#as: --32
#strip:
#readelf: -S --wide
#name: strip on uncompressed debug sections

There are 5 section headers, starting at offset 0x78:

Section Headers:
  \[Nr\] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  \[ 0\]                   NULL            00000000 000000 000000 00      0   0  0
  \[ 1\] .text             PROGBITS        00000000 000040 00001b 00  AX  0   0 16
  \[ 2\] .data             PROGBITS        00000000 00005c 000000 00  WA  0   0  4
  \[ 3\] .bss              NOBITS          00000000 00005c 000000 00  WA  0   0  4
  \[ 4\] .shstrtab         STRTAB          00000000 00005c 00001c 00      0   0  1
Key to Flags:
#...
