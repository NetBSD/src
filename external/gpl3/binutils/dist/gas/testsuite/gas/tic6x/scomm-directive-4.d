#name: C6X .scomm directive 4
#as:
#source: scomm-directive-4.s
#readelf: -Ss

There are 8 section headers, starting at offset 0x88:

Section Headers:
  \[Nr\] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  \[ 0\]                   NULL            00000000 000000 000000 00      0   0  0
  \[ 1\] \.text             PROGBITS        00000000 000034 000000 00  AX  0   0  1
  \[ 2\] \.data             PROGBITS        00000000 000034 000000 00  WA  0   0  1
  \[ 3\] \.bss              NOBITS          00000000 000034 000000 00  WA  0   0  1
  \[ 4\] \.c6xabi\.attribute C6000_ATTRIBUTE 00000000 000034 000013 00      0   0  1
  \[ 5\] \.shstrtab         STRTAB          00000000 000047 00003f 00      0   0  1
  \[ 6\] \.symtab           SYMTAB          00000000 0001c8 0000d0 10      7   5  4
  \[ 7\] \.strtab           STRTAB          00000000 000298 00001d 00      0   0  1
Key to Flags:
  W \(write\), A \(alloc\), X \(execute\), M \(merge\), S \(strings\)
  I \(info\), L \(link order\), G \(group\), T \(TLS\), E \(exclude\), x \(unknown\)
  O \(extra OS processing required\) o \(OS specific\), p \(processor specific\)

Symbol table '\.symtab' contains 13 entries:
   Num:    Value  Size Type    Bind   Vis      Ndx Name
     0: 00000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 00000000     0 SECTION LOCAL  DEFAULT    1 
     2: 00000000     0 SECTION LOCAL  DEFAULT    2 
     3: 00000000     0 SECTION LOCAL  DEFAULT    3 
     4: 00000000     0 SECTION LOCAL  DEFAULT    4 
     5: 00000004     4 OBJECT  GLOBAL DEFAULT  COM x4a
     6: 00000004     4 OBJECT  GLOBAL DEFAULT SCOM y4a
     7: 00000002     4 OBJECT  GLOBAL DEFAULT  COM x4b
     8: 00000002     4 OBJECT  GLOBAL DEFAULT SCOM y4b
     9: 00000004     2 OBJECT  GLOBAL DEFAULT  COM x2
    10: 00000004     2 OBJECT  GLOBAL DEFAULT SCOM y2
    11: 00000004     1 OBJECT  GLOBAL DEFAULT  COM x1
    12: 00000004     1 OBJECT  GLOBAL DEFAULT SCOM y1
