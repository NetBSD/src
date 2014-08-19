There are 17 section headers, starting at offset 0x21c4:

Section Headers:
  \[Nr\] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  \[ 0\]                   NULL            00000000 000000 000000 00      0   0  0
  \[ 1\] \.hash             HASH            00008000 001000 000048 04   A  2   0  4
  \[ 2\] \.dynsym           DYNSYM          00008048 001048 0000d0 10   A  3   6  4
  \[ 3\] \.dynstr           STRTAB          00008118 001118 000025 00   A  0   0  1
  \[ 4\] \.rela\.got         RELA            00008140 001140 000024 0c   A  2  10  4
  \[ 5\] \.rela\.neardata    RELA            00008164 001164 000018 0c   A  2  11  4
  \[ 6\] \.dynamic          DYNAMIC         0000817c 00117c 0000a8 08  WA  3   0  4
  \[ 7\] \.rela\.plt         RELA            10000000 002000 000018 0c   A  2   8  4
  \[ 8\] \.plt              PROGBITS        10000020 002020 000060 18  AX  0   0 32
  \[ 9\] \.text             PROGBITS        10000080 002080 000080 00  AX  0   0 32
  \[10\] \.got              PROGBITS        10000100 002100 000028 00  WA  0   0  4
  \[11\] \.neardata         PROGBITS        10000128 002128 000008 00  WA  0   0  4
  \[12\] \.bss              NOBITS          10000130 002130 000004 00  WA  0   0  4
  \[13\] \.c6xabi\.attributes C6000_ATTRIBUTES 00000000 002130 000019 00      0   0  1
  \[14\] \.shstrtab         STRTAB          00000000 002149 00007b 00      0   0  1
  \[15\] \.symtab           SYMTAB          00000000 [0-9a-f]+ [0-9a-f]+ 10     16  [0-9]+  4
  \[16\] \.strtab           STRTAB          00000000 [0-9a-f]+ [0-9a-f]+ 00      0   0  1
Key to Flags:
  W \(write\), A \(alloc\), X \(execute\), M \(merge\), S \(strings\)
  I \(info\), L \(link order\), G \(group\), T \(TLS\), E \(exclude\), x \(unknown\)
  O \(extra OS processing required\) o \(OS specific\), p \(processor specific\)

Elf file type is DYN \(Shared object file\)
Entry point 0x10000080
There are 4 program headers, starting at offset 52

Program Headers:
  Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align
  LOAD           0x001000 0x00008000 0x00008000 0x00224 0x00224 RW  0x1000
  LOAD           0x002000 0x10000000 0x10000000 0x00130 0x00134 RWE 0x1000
  DYNAMIC        0x00117c 0x0000817c 0x0000817c 0x000a8 0x000a8 RW  0x4
  GNU_STACK      0x000000 0x00000000 0x00000000 0x00000 0x20000 RWE 0x8

 Section to Segment mapping:
  Segment Sections\.\.\.
   00     \.hash \.dynsym \.dynstr \.rela\.got \.rela\.neardata \.dynamic 
   01     \.rela\.plt \.plt \.text \.got \.neardata \.bss 
   02     \.dynamic 
   03     

Dynamic section at offset 0x117c contains 16 entries:
  Tag        Type                         Name/Value
 0x00000004 \(HASH\)                       0x8000
 0x00000005 \(STRTAB\)                     0x8118
 0x00000006 \(SYMTAB\)                     0x8048
 0x0000000a \(STRSZ\)                      37 \(bytes\)
 0x0000000b \(SYMENT\)                     16 \(bytes\)
 0x70000000 \(C6000_DSBT_BASE\)            0x10000100
 0x70000001 \(C6000_DSBT_SIZE\)            0x3
 0x70000003 \(C6000_DSBT_INDEX\)           0x2
 0x00000003 \(PLTGOT\)                     0x1000010c
 0x00000002 \(PLTRELSZ\)                   24 \(bytes\)
 0x00000014 \(PLTREL\)                     RELA
 0x00000017 \(JMPREL\)                     0x10000000
 0x00000007 \(RELA\)                       0x8140
 0x00000008 \(RELASZ\)                     84 \(bytes\)
 0x00000009 \(RELAENT\)                    12 \(bytes\)
 0x00000000 \(NULL\)                       0x0

Relocation section '\.rela\.got' at offset 0x1140 contains 3 entries:
 Offset     Info    Type                Sym\. Value  Symbol's Name \+ Addend
10000120  00000501 R_C6000_ABS32          10000130   \.bss \+ 0
1000011c  00000701 R_C6000_ABS32          00000000   b \+ 0
10000124  00000b01 R_C6000_ABS32          10000128   a \+ 0

Relocation section '\.rela\.neardata' at offset 0x1164 contains 2 entries:
 Offset     Info    Type                Sym\. Value  Symbol's Name \+ Addend
10000128  00000a01 R_C6000_ABS32          10000088   sub0 \+ 0
1000012c  00000801 R_C6000_ABS32          00000000   g1 \+ 0

Relocation section '\.rela\.plt' at offset 0x2000 contains 2 entries:
 Offset     Info    Type                Sym\. Value  Symbol's Name \+ Addend
10000114  00000a1b R_C6000_JUMP_SLOT      10000088   sub0 \+ 0
10000118  00000c1b R_C6000_JUMP_SLOT      100000c0   sub \+ 0

Symbol table '\.dynsym' contains 13 entries:
   Num:    Value  Size Type    Bind   Vis      Ndx Name
     0: 00000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 10000020     0 SECTION LOCAL  DEFAULT    8 
     2: 10000080     0 SECTION LOCAL  DEFAULT    9 
     3: 10000100     0 SECTION LOCAL  DEFAULT   10 
     4: 10000128     0 SECTION LOCAL  DEFAULT   11 
     5: 10000130     0 SECTION LOCAL  DEFAULT   12 
     6: 10000100     0 NOTYPE  LOCAL  DEFAULT   10 __c6xabi_DSBT_BASE
     7: 00000000     0 NOTYPE  WEAK   DEFAULT  UND b
     8: 00000000     0 NOTYPE  WEAK   DEFAULT  UND g1
     9: 1000012c     4 OBJECT  GLOBAL DEFAULT   11 g2
    10: 10000088    52 FUNC    GLOBAL DEFAULT    9 sub0
    11: 10000128     4 OBJECT  GLOBAL DEFAULT   11 a
    12: 100000c0    52 FUNC    GLOBAL DEFAULT    9 sub

Symbol table '\.symtab' contains [0-9]+ entries:
   Num:    Value  Size Type    Bind   Vis      Ndx Name
.* 00000000     0 NOTYPE  LOCAL  DEFAULT  UND 
.* 00008000     0 SECTION LOCAL  DEFAULT    1 
.* 00008048     0 SECTION LOCAL  DEFAULT    2 
.* 00008118     0 SECTION LOCAL  DEFAULT    3 
.* 00008140     0 SECTION LOCAL  DEFAULT    4 
.* 00008164     0 SECTION LOCAL  DEFAULT    5 
.* 0000817c     0 SECTION LOCAL  DEFAULT    6 
.* 10000000     0 SECTION LOCAL  DEFAULT    7 
.* 10000020     0 SECTION LOCAL  DEFAULT    8 
.* 10000080     0 SECTION LOCAL  DEFAULT    9 
.* 10000100     0 SECTION LOCAL  DEFAULT   10 
.* 10000128     0 SECTION LOCAL  DEFAULT   11 
.* 10000130     0 SECTION LOCAL  DEFAULT   12 
.* 00000000     0 SECTION LOCAL  DEFAULT   13 
.* 00000000     0 FILE    LOCAL  DEFAULT  ABS .*
.* 10000080     0 FUNC    LOCAL  HIDDEN     9 sub1
.* 10000130     4 OBJECT  LOCAL  DEFAULT   12 c
.* 00000000     0 FILE    LOCAL  DEFAULT  ABS .*
.* 0000817c     0 OBJECT  LOCAL  DEFAULT  ABS _DYNAMIC
.* 1000010c     0 OBJECT  LOCAL  DEFAULT  ABS _GLOBAL_OFFSET_TABLE_
.* 10000100     0 NOTYPE  LOCAL  DEFAULT   10 __c6xabi_DSBT_BASE
.* 00000000     0 NOTYPE  WEAK   DEFAULT  UND b
.* 00020000     0 OBJECT  GLOBAL DEFAULT  ABS __stacksize
.* 00000000     0 NOTYPE  WEAK   DEFAULT  UND g1
.* 1000012c     4 OBJECT  GLOBAL DEFAULT   11 g2
.* 10000088    52 FUNC    GLOBAL DEFAULT    9 sub0
.* 10000128     4 OBJECT  GLOBAL DEFAULT   11 a
.* 100000c0    52 FUNC    GLOBAL DEFAULT    9 sub
