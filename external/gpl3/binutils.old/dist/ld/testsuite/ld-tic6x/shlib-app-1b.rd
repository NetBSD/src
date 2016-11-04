There are 18 section headers, starting at offset .*:

Section Headers:
  \[Nr\] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  \[ 0\]                   NULL            00000000 000000 000000 00      0   0  0
  \[ 1\] \.hash             HASH            00008000 001000 000044 04   A  2   0  4
  \[ 2\] \.dynsym           DYNSYM          00008044 001044 0000c0 10   A  3   6  4
  \[ 3\] \.dynstr           STRTAB          00008104 001104 000036 00   A  0   0  1
  \[ 4\] \.rela\.got         RELA            0000813c 00113c 000018 0c  AI  2  11  4
  \[ 5\] \.rela\.neardata    RELA            00008154 001154 000018 0c  AI  2  12  4
  \[ 6\] \.rela\.bss         RELA            0000816c 00116c 00000c 0c  AI  2  13  4
  \[ 7\] \.dynamic          DYNAMIC         00008178 001178 0000b8 08  WA  3   0  4
  \[ 8\] \.rela\.plt         RELA            10000000 002000 00000c 0c  AI  2  11  4
  \[ 9\] \.plt              PROGBITS        10000020 002020 000040 18  AX  0   0 32
  \[10\] \.text             PROGBITS        10000060 002060 000040 00  AX  0   0 32
  \[11\] \.got              PROGBITS        100000a0 0020a0 000020 00  WA  0   0  4
  \[12\] \.neardata         PROGBITS        100000c0 0020c0 00000c 00  WA  0   0  4
  \[13\] \.bss              NOBITS          100000cc 0020cc 000004 00  WA  0   0  4
  \[14\] \.c6xabi\.attributes C6000_ATTRIBUTES 00000000 0020cc 000019 00      0   0  1
  \[15\] \.shstrtab         STRTAB          00000000 [0-9a-f]+ 000080 00      0   0  1
  \[16\] \.symtab           SYMTAB          00000000 [0-9a-f]+ [0-9a-f]+ 10     17  [0-9]+  4
  \[17\] \.strtab           STRTAB          00000000 [0-9a-f]+ [0-9a-f]+ 00      0   0  1
Key to Flags:
  W \(write\), A \(alloc\), X \(execute\), M \(merge\), S \(strings\)
  I \(info\), L \(link order\), G \(group\), T \(TLS\), E \(exclude\), x \(unknown\)
  O \(extra OS processing required\) o \(OS specific\), p \(processor specific\)

Elf file type is EXEC \(Executable file\)
Entry point 0x10000060
There are 4 program headers, starting at offset 52

Program Headers:
  Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align
  LOAD           0x001000 0x00008000 0x00008000 0x00230 0x00230 RW  0x1000
  LOAD           0x002000 0x10000000 0x10000000 0x000cc 0x000d0 RWE 0x1000
  DYNAMIC        0x001178 0x00008178 0x00008178 0x000b8 0x000b8 RW  0x4
  GNU_STACK      0x000000 0x00000000 0x00000000 0x00000 0x20000 RWE 0x8

 Section to Segment mapping:
  Segment Sections\.\.\.
   00     \.hash \.dynsym \.dynstr \.rela\.got \.rela\.neardata \.rela\.bss \.dynamic 
   01     \.rela\.plt \.plt \.text \.got \.neardata \.bss 
   02     \.dynamic 
   03     

Dynamic section at offset 0x1178 contains 18 entries:
  Tag        Type                         Name/Value
 0x00000001 \(NEEDED\)                     Shared library: \[tmpdir/libtestb\.so\]
 0x00000004 \(HASH\)                       0x8000
 0x00000005 \(STRTAB\)                     0x8104
 0x00000006 \(SYMTAB\)                     0x8044
 0x0000000a \(STRSZ\)                      54 \(bytes\)
 0x0000000b \(SYMENT\)                     16 \(bytes\)
 0x00000015 \(DEBUG\)                      0x0
 0x70000000 \(C6000_DSBT_BASE\)            0x100000a0
 0x70000001 \(C6000_DSBT_SIZE\)            0x3
 0x70000003 \(C6000_DSBT_INDEX\)           0x0
 0x00000003 \(PLTGOT\)                     0x100000ac
 0x00000002 \(PLTRELSZ\)                   12 \(bytes\)
 0x00000014 \(PLTREL\)                     RELA
 0x00000017 \(JMPREL\)                     0x10000000
 0x00000007 \(RELA\)                       0x813c
 0x00000008 \(RELASZ\)                     72 \(bytes\)
 0x00000009 \(RELAENT\)                    12 \(bytes\)
 0x00000000 \(NULL\)                       0x0

Relocation section '\.rela\.got' at offset 0x113c contains 2 entries:
 Offset     Info    Type                Sym\. Value  Symbol's Name \+ Addend
100000b8  00000701 R_C6000_ABS32          100000c0   b \+ 0
100000bc  00000b01 R_C6000_ABS32          100000cc   a \+ 0

Relocation section '\.rela\.neardata' at offset 0x1154 contains 2 entries:
 Offset     Info    Type                Sym\. Value  Symbol's Name \+ Addend
100000c4  00000801 R_C6000_ABS32          00000000   g1 \+ 0
100000c8  00000901 R_C6000_ABS32          00000000   g2 \+ 0

Relocation section '\.rela\.bss' at offset 0x116c contains 1 entries:
 Offset     Info    Type                Sym\. Value  Symbol's Name \+ Addend
100000cc  00000b1a R_C6000_COPY           100000cc   a \+ 0

Relocation section '\.rela\.plt' at offset 0x2000 contains 1 entries:
 Offset     Info    Type                Sym\. Value  Symbol's Name \+ Addend
100000b4  00000a1b R_C6000_JUMP_SLOT      00000000   sub0 \+ 0

Symbol table '\.dynsym' contains 12 entries:
   Num:    Value  Size Type    Bind   Vis      Ndx Name
     0: 00000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 10000020     0 SECTION LOCAL  DEFAULT    9 
     2: 10000060     0 SECTION LOCAL  DEFAULT   10 
     3: 100000a0     0 SECTION LOCAL  DEFAULT   11 
     4: 100000c0     0 SECTION LOCAL  DEFAULT   12 
     5: 100000cc     0 SECTION LOCAL  DEFAULT   13 
     6: 100000a0     0 NOTYPE  LOCAL  DEFAULT   11 __c6xabi_DSBT_BASE
     7: 100000c0     4 OBJECT  GLOBAL DEFAULT   12 b
     8: 00000000     0 NOTYPE  WEAK   DEFAULT  UND g1
     9: 00000000     0 OBJECT  WEAK   DEFAULT  UND g2
    10: 00000000     0 FUNC    GLOBAL DEFAULT  UND sub0
    11: 100000cc     4 OBJECT  GLOBAL DEFAULT   13 a

Symbol table '\.symtab' contains [0-9]+ entries:
   Num:    Value  Size Type    Bind   Vis      Ndx Name
.* 00000000     0 NOTYPE  LOCAL  DEFAULT  UND 
.* 00008000     0 SECTION LOCAL  DEFAULT    1 
.* 00008044     0 SECTION LOCAL  DEFAULT    2 
.* 00008104     0 SECTION LOCAL  DEFAULT    3 
.* 0000813c     0 SECTION LOCAL  DEFAULT    4 
.* 00008154     0 SECTION LOCAL  DEFAULT    5 
.* 0000816c     0 SECTION LOCAL  DEFAULT    6 
.* 00008178     0 SECTION LOCAL  DEFAULT    7 
.* 10000000     0 SECTION LOCAL  DEFAULT    8 
.* 10000020     0 SECTION LOCAL  DEFAULT    9 
.* 10000060     0 SECTION LOCAL  DEFAULT   10 
.* 100000a0     0 SECTION LOCAL  DEFAULT   11 
.* 100000c0     0 SECTION LOCAL  DEFAULT   12 
.* 100000cc     0 SECTION LOCAL  DEFAULT   13 
.* 00000000     0 SECTION LOCAL  DEFAULT   14 
.* 00000000     0 FILE    LOCAL  DEFAULT  ABS .*
.* 10000060     0 NOTYPE  LOCAL  DEFAULT   10 fish
.* 100000c4     8 OBJECT  LOCAL  DEFAULT   12 w
.* 00000000     0 FILE    LOCAL  DEFAULT  ABS .*
.* 00008178     0 OBJECT  LOCAL  DEFAULT    7 _DYNAMIC
.* 100000ac     0 OBJECT  LOCAL  DEFAULT   11 _GLOBAL_OFFSET_TABLE_
.* 100000a0     0 NOTYPE  LOCAL  DEFAULT   11 __c6xabi_DSBT_BASE
.* 100000c0     4 OBJECT  GLOBAL DEFAULT   12 b
.* 00000000     0 NOTYPE  WEAK   DEFAULT  UND g1
.* 00000000     0 OBJECT  WEAK   DEFAULT  UND g2
.* 00000000     0 FUNC    GLOBAL DEFAULT  UND sub0
.* 100000cc     4 OBJECT  GLOBAL DEFAULT   13 a
