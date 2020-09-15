#as: -mx86-used-note=no --generate-missing-build-notes=no
#readelf: -S
#name: x86-64 unwind

There are 8 section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
  \[Nr\] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  \[ 0\]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  \[ 1\] \.text             PROGBITS         0000000000000000  00000040
       0000000000000000  0000000000000000  AX       0     0     1
  \[ 2\] \.data             PROGBITS         0000000000000000  00000040
       0000000000000000  0000000000000000  WA       0     0     1
  \[ 3\] \.bss              NOBITS           0000000000000000  00000040
       0000000000000000  0000000000000000  WA       0     0     1
  \[ 4\] \.eh_frame         X86_64_UNWIND    0000000000000000  00000040
       0000000000000008  0000000000000000   A       0     0     1
  \[ 5\] \.symtab           SYMTAB           0000000000000000  [0-9a-f]+
       0000000000000078  0000000000000018           6     5     8
  \[ 6\] \.strtab           STRTAB           0000000000000000  [0-9a-f]+
       0000000000000001  0000000000000000   .       0     0     1
  \[ 7\] \.shstrtab         STRTAB           0000000000000000  [0-9a-f]+
       0000000000000036  0000000000000000   .       0     0     1
Key to Flags:
#...
