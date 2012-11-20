#source: bspec1.s
#source: start.s
#ld: -m elf64mmix
#readelf: -Ssr -x1 -x5

There are 9 section headers, starting at offset 0x100:

Section Headers:
  \[Nr\] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  \[ 0\]                   NULL             0+  0+
       0+  0+           0     0     0
  \[ 1\] \.text             PROGBITS         0+  0+b0
       0+4  0+  AX       0     0     4
  \[ 2\] \.data             PROGBITS         2000000000000000  0+b4
       0+  0+  WA       0     0     1
  \[ 3\] \.sbss             PROGBITS         2000000000000000  0+b4
       0+  0+   W       0     0     1
  \[ 4\] \.bss              NOBITS           2000000000000000  0+b4
       0+  0+  WA       0     0     1
  \[ 5\] \.MMIX\.spec_data\.2 PROGBITS         0+  0+b4
       0+4  0+           0     0     4
  \[ 6\] \.shstrtab         STRTAB           0+  0+b8
       0+44  0+           0     0     1
  \[ 7\] \.symtab           SYMTAB           0+  0+340
       0+168  0+18           8     9     8
  \[ 8\] \.strtab           STRTAB           0+  0+4a8
       0+2d  0+           0     0     1
Key to Flags:
  W \(write\), A \(alloc\), X \(execute\), M \(merge\), S \(strings\)
  I \(info\), L \(link order\), G \(group\), x \(unknown\)
  O \(extra OS processing required\) o \(OS specific\), p \(processor specific\)

There are no relocations in this file\.

Symbol table '\.symtab' contains 15 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0+     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0+     0 SECTION LOCAL  DEFAULT    1 
     2: 2000000000000000     0 SECTION LOCAL  DEFAULT    2 
     3: 2000000000000000     0 SECTION LOCAL  DEFAULT    3 
     4: 2000000000000000     0 SECTION LOCAL  DEFAULT    4 
     5: 0+     0 SECTION LOCAL  DEFAULT    5 
     6: 0+     0 SECTION LOCAL  DEFAULT    6 
     7: 0+     0 SECTION LOCAL  DEFAULT    7 
     8: 0+     0 SECTION LOCAL  DEFAULT    8 
     9: 0+     0 FUNC    GLOBAL DEFAULT    1 Main
    10: 0+     0 NOTYPE  GLOBAL DEFAULT    1 _start
#...

Hex dump of section '\.text':
  0x0+ e3fd0001                            .*

Hex dump of section '\.MMIX\.spec_data\.2':
  0x0+ 0000002a                            .*
