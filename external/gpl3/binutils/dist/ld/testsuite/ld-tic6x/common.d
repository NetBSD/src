#name: C6X common symbols
#as: -mlittle-endian
#ld: -melf32_tic6x_le -Tcommon.ld
#source: common.s
#readelf: -Ss

There are 6 section headers, starting at offset 0x7c:

Section Headers:
  \[Nr\] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  \[ 0\]                   NULL            00000000 000000 000000 00      0   0  0
  \[ 1\] \.far              NOBITS          00000080 000080 000008 00  WA  0   0  4
  \[ 2\] \.bss              NOBITS          00000100 000080 000004 00  WA  0   0  4
  \[ 3\] \.shstrtab         STRTAB          00000000 000054 000025 00      0   0  1
  \[ 4\] \.symtab           SYMTAB          00000000 00016c 000050 10      5   3  4
  \[ 5\] \.strtab           STRTAB          00000000 0001bc 000005 00      0   0  1
Key to Flags:
  W \(write\), A \(alloc\), X \(execute\), M \(merge\), S \(strings\)
  I \(info\), L \(link order\), G \(group\), T \(TLS\), E \(exclude\), x \(unknown\)
  O \(extra OS processing required\) o \(OS specific\), p \(processor specific\)

Symbol table '\.symtab' contains 5 entries:
   Num:    Value  Size Type    Bind   Vis      Ndx Name
     0: 00000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 00000080     0 SECTION LOCAL  DEFAULT    1 
     2: 00000100     0 SECTION LOCAL  DEFAULT    2 
     3: 00000100     4 OBJECT  GLOBAL DEFAULT    2 x
     4: 00000080     8 OBJECT  GLOBAL DEFAULT    1 y
