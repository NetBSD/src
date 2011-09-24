#PROG: strip
#source: compressed-1.s
#as: --64
#strip:
#readelf: -S --wide
#name: strip on uncompressed debug sections

There are 6 section headers, starting at offset 0x80:

Section Headers:
  \[Nr\] Name              Type            Address          Off    Size   ES Flg Lk Inf Al
  \[ 0\]                   NULL            0000000000000000 000000 000000 00      0   0  0
  \[ 1\] .text             PROGBITS        0000000000000000 000040 000015 00  AX  0   0 16
  \[ 2\] .rela.text        RELA            0000000000000000 000200 000000 18      0   1  8
  \[ 3\] .data             PROGBITS        0000000000000000 000058 000000 00  WA  0   0  4
  \[ 4\] .bss              NOBITS          0000000000000000 000058 000000 00  WA  0   0  4
  \[ 5\] .shstrtab         STRTAB          0000000000000000 000058 000021 00      0   0  1
Key to Flags:
#...
