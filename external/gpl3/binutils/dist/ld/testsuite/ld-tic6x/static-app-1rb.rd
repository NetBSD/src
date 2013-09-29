There are 15 section headers, starting at offset 0x2178:

Section Headers:
  \[Nr\] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  \[ 0\]                   NULL            00000000 000000 000000 00      0   0  0
  \[ 1\] \.hash             HASH            00008000 001000 00003c 04   A  2   0  4
  \[ 2\] \.dynsym           DYNSYM          0000803c 00103c 0000a0 10   A  3   5  4
  \[ 3\] \.dynstr           STRTAB          000080dc 0010dc 00001d 00   A  0   0  1
  \[ 4\] \.rela\.got         RELA            000080fc 0010fc 000024 0c   A  2   8  4
  \[ 5\] \.rela\.neardata    RELA            00008120 001120 000018 0c   A  2   9  4
  \[ 6\] \.dynamic          DYNAMIC         00008138 001138 000090 08  WA  3   0  4
  \[ 7\] \.text             PROGBITS        10000000 002000 0000c0 00  AX  0   0 32
  \[ 8\] \.got              PROGBITS        100000c0 0020c0 000020 00  WA  0   0  4
  \[ 9\] \.neardata         PROGBITS        100000e0 0020e0 00000c 00  WA  0   0  4
  \[10\] \.bss              NOBITS          100000ec 0020ec 000004 00  WA  0   0  4
  \[11\] \.c6xabi\.attributes C6000_ATTRIBUTES 00000000 0020ec 000019 00      0   0  1
  \[12\] \.shstrtab         STRTAB          00000000 002105 000071 00      0   0  1
  \[13\] \.symtab           SYMTAB          00000000 [0-9a-f]+ [0-9a-f]+ 10     14  [0-9]+  4
  \[14\] \.strtab           STRTAB          00000000 [0-9a-f]+ [0-9a-f]+ 00      0   0  1
Key to Flags:
  W \(write\), A \(alloc\), X \(execute\), M \(merge\), S \(strings\)
  I \(info\), L \(link order\), G \(group\), T \(TLS\), E \(exclude\), x \(unknown\)
  O \(extra OS processing required\) o \(OS specific\), p \(processor specific\)

Elf file type is EXEC \(Executable file\)
Entry point 0x10000000
There are 4 program headers, starting at offset 52

Program Headers:
  Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align
  LOAD           0x001000 0x00008000 0x00008000 0x001c8 0x001c8 RW  0x1000
  LOAD           0x002000 0x10000000 0x10000000 0x000ec 0x000f0 RWE 0x1000
  DYNAMIC        0x001138 0x00008138 0x00008138 0x00090 0x00090 RW  0x4
  GNU_STACK      0x000000 0x00000000 0x00000000 0x00000 0x20000 RWE 0x8

 Section to Segment mapping:
  Segment Sections\.\.\.
   00     \.hash \.dynsym \.dynstr \.rela\.got \.rela\.neardata \.dynamic 
   01     \.text \.got \.neardata \.bss 
   02     \.dynamic 
   03     

Dynamic section at offset 0x1138 contains 13 entries:
  Tag        Type                         Name/Value
 0x00000004 \(HASH\)                       0x8000
 0x00000005 \(STRTAB\)                     0x80dc
 0x00000006 \(SYMTAB\)                     0x803c
 0x0000000a \(STRSZ\)                      29 \(bytes\)
 0x0000000b \(SYMENT\)                     16 \(bytes\)
 0x00000015 \(DEBUG\)                      0x0
 0x70000000 \(C6000_DSBT_BASE\)            0x100000c0
 0x70000001 \(C6000_DSBT_SIZE\)            0x3
 0x70000003 \(C6000_DSBT_INDEX\)           0x0
 0x00000007 \(RELA\)                       0x80fc
 0x00000008 \(RELASZ\)                     60 \(bytes\)
 0x00000009 \(RELAENT\)                    12 \(bytes\)
 0x00000000 \(NULL\)                       0x0

Relocation section '\.rela\.got' at offset 0x10fc contains 3 entries:
 Offset     Info    Type                Sym\. Value  Symbol's Name \+ Addend
100000d8  00000401 R_C6000_ABS32          100000ec   \.bss \+ 0
100000d4  00000701 R_C6000_ABS32          100000e8   b \+ 0
100000dc  00000901 R_C6000_ABS32          100000e0   a \+ 0

Relocation section '\.rela\.neardata' at offset 0x1120 contains 2 entries:
 Offset     Info    Type                Sym\. Value  Symbol's Name \+ Addend
100000e0  00000101 R_C6000_ABS32          10000000   \.text \+ 8
100000e4  00000801 R_C6000_ABS32          00000000   g1 \+ 0

Symbol table '\.dynsym' contains 10 entries:
   Num:    Value  Size Type    Bind   Vis      Ndx Name
     0: 00000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 10000000     0 SECTION LOCAL  DEFAULT    7 
     2: 100000c0     0 SECTION LOCAL  DEFAULT    8 
     3: 100000e0     0 SECTION LOCAL  DEFAULT    9 
     4: 100000ec     0 SECTION LOCAL  DEFAULT   10 
     5: 100000ec     4 OBJECT  LOCAL  DEFAULT   10 c
     6: 100000c0     0 NOTYPE  LOCAL  DEFAULT    8 __c6xabi_DSBT_BASE
     7: 100000e8     4 OBJECT  GLOBAL DEFAULT    9 b
     8: 00000000     0 NOTYPE  WEAK   DEFAULT  UND g1
     9: 100000e0     4 OBJECT  GLOBAL DEFAULT    9 a

Symbol table '\.symtab' contains [0-9]+ entries:
   Num:    Value  Size Type    Bind   Vis      Ndx Name
.* 00000000     0 NOTYPE  LOCAL  DEFAULT  UND 
.* 00008000     0 SECTION LOCAL  DEFAULT    1 
.* 0000803c     0 SECTION LOCAL  DEFAULT    2 
.* 000080dc     0 SECTION LOCAL  DEFAULT    3 
.* 000080fc     0 SECTION LOCAL  DEFAULT    4 
.* 00008120     0 SECTION LOCAL  DEFAULT    5 
.* 00008138     0 SECTION LOCAL  DEFAULT    6 
.* 10000000     0 SECTION LOCAL  DEFAULT    7 
.* 100000c0     0 SECTION LOCAL  DEFAULT    8 
.* 100000e0     0 SECTION LOCAL  DEFAULT    9 
.* 100000ec     0 SECTION LOCAL  DEFAULT   10 
.* 00000000     0 SECTION LOCAL  DEFAULT   11 
.* 00000000     0 FILE    LOCAL  DEFAULT  ABS .*
.* 10000000     0 FUNC    LOCAL  HIDDEN     7 sub1
.* 00000000     0 FILE    LOCAL  DEFAULT  ABS .*
.* 10000080     0 NOTYPE  LOCAL  DEFAULT    7 fish
.* 00000000     0 FILE    LOCAL  DEFAULT  ABS .*
.* 00008138     0 OBJECT  LOCAL  DEFAULT    6 _DYNAMIC
.* 100000ec     4 OBJECT  LOCAL  DEFAULT   10 c
.* 100000cc     0 OBJECT  LOCAL  DEFAULT    8 _GLOBAL_OFFSET_TABLE_
.* 100000c0     0 NOTYPE  LOCAL  DEFAULT    8 __c6xabi_DSBT_BASE
.* 100000e8     4 OBJECT  GLOBAL DEFAULT    9 b
.* 00020000     0 OBJECT  GLOBAL DEFAULT  ABS __stacksize
.* 00000000     0 NOTYPE  WEAK   DEFAULT  UND g1
.* 100000e4     4 OBJECT  GLOBAL DEFAULT    9 g2
.* 10000008    52 FUNC    GLOBAL DEFAULT    7 sub0
.* 100000e0     4 OBJECT  GLOBAL DEFAULT    9 a
.* 10000040    52 FUNC    GLOBAL DEFAULT    7 sub
