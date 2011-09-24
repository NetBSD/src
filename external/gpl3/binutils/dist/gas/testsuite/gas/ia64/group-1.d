#readelf: -Sg
#name: ia64 group

There are 9 section headers, starting at offset 0x98:

Section Headers:
  \[Nr\] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  \[ 0\]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  \[ 1\] \.group            GROUP            0000000000000000  00000040
       0000000000000008  0000000000000004           7     6     4
  \[ 2\] \.text             PROGBITS         0000000000000000  00000050
       0000000000000000  0000000000000000  AX       0     0     16
  \[ 3\] \.data             PROGBITS         0000000000000000  00000050
       0000000000000000  0000000000000000  WA       0     0     1
  \[ 4\] \.bss              NOBITS           0000000000000000  00000050
       0000000000000000  0000000000000000  WA       0     0     1
  \[ 5\] \.text             PROGBITS         0000000000000000  00000050
       0000000000000010  0000000000000000 AXG       0     0     16
  \[ 6\] \.shstrtab         STRTAB           0000000000000000  00000060
       0000000000000033  0000000000000000           0     0     1
  \[ 7\] \.symtab           SYMTAB           0000000000000000  000002d8
       00000000000000c0  0000000000000018           8     8     8
  \[ 8\] \.strtab           STRTAB           0000000000000000  00000398
       000000000000000c  0000000000000000           0     0     1
Key to Flags:
#...

COMDAT group section \[    1\] `\.group' \[\._foo\] contains 1 sections:
   \[Index\]    Name
   \[    5\]   \.text
