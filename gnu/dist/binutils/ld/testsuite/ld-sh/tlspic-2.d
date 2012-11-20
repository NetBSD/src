#source: tlspic1.s
#source: tlspic2.s
#as: -little
#ld: -shared -EL
#readelf: -Ssrl
#target: sh*-*-linux* sh*-*-netbsd*

There are [0-9]+ section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
  \[Nr\] Name +Type +Addr +Off +Size +ES Flg Lk Inf Al
  \[ 0\] +NULL +0+ 0+ 0+ 0+ +0 +0 +0
  \[ 1\] \.hash +.*
  \[ 2\] \.dynsym +.*
  \[ 3\] \.dynstr +.*
  \[ 4\] \.rela\.dyn +.*
  \[ 5\] \.rela\.plt +.*
  \[ 6\] \.plt +.*
  \[ 7\] \.text +PROGBITS +0+[0-9a-f]+ .*
  \[ 8\] \.tdata +PROGBITS +0+[0-9a-f]+ [0-9a-f]+ 0+018 00 WAT  0   0  4
  \[ 9\] \.tbss +NOBITS +0+[0-9a-f]+ [0-9a-f]+ 0+008 00 WAT  0   0  1
#...
  \[[0-9a-f]+\] \.got +PROGBITS +0+[0-9a-f]+ .*
  \[[0-9a-f]+\] \.sbss +.*
  \[[0-9a-f]+\] \.bss +.*
#...
  \[[0-9a-f]+\] \.shstrtab +.*
  \[[0-9a-f]+\] \.symtab +.*
  \[[0-9a-f]+\] \.strtab +.*
Key to Flags:
.*
.*
.*

Elf file type is DYN \(Shared object file\)
Entry point 0x[0-9a-f]+
There are 4 program headers, starting at offset [0-9]+

Program Headers:
  Type +Offset +VirtAddr +PhysAddr +FileSiz +MemSiz +Flg Align
  LOAD.*
  LOAD.*
  DYNAMIC.*
  TLS +0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ 0x0+18 0x0+20 R +0x4

 Section to Segment mapping:
  Segment Sections\.\.\.
   00 +\.hash \.dynsym \.dynstr \.rela\.dyn \.rela\.plt \.plt \.text *
   01 +\.tdata \.dynamic \.got *
   02 +\.dynamic *
   03 +\.tdata \.tbss *

Relocation section '\.rela\.dyn' at offset 0x[0-9a-f]+ contains 10 entries:
 Offset +Info +Type +Sym\.Value +Sym\. Name \+ Addend
0+[0-9a-f]+  00000095 R_SH_TLS_DTPMOD32 +0+00
0+[0-9a-f]+  00000097 R_SH_TLS_TPOFF32 +0+0c
0+[0-9a-f]+  00000095 R_SH_TLS_DTPMOD32 +0+00
0+[0-9a-f]+  00000095 R_SH_TLS_DTPMOD32 +0+00
0+[0-9a-f]+  00000097 R_SH_TLS_TPOFF32 +0+1c
0+[0-9a-f]+  00000095 R_SH_TLS_DTPMOD32 +0+00
0+[0-9a-f]+  00000097 R_SH_TLS_TPOFF32 +0+14
0+[0-9a-f]+  0000[0-9a-f]+95 R_SH_TLS_DTPMOD32 +0+ +sg1 \+ 0
0+[0-9a-f]+  0000[0-9a-f]+96 R_SH_TLS_DTPOFF32 +0+ +sg1 \+ 0
0+[0-9a-f]+  0000[0-9a-f]+97 R_SH_TLS_TPOFF32 +0+04 +sg2 \+ 0

Relocation section '\.rela\.plt' at offset 0x[0-9a-f]+ contains 1 entries:
 Offset +Info +Type +Sym\.Value +Sym\. Name \+ Addend
0+[0-9a-f]+  0000[0-9a-f]+a4 R_SH_JMP_SLOT +[0-9a-f]+ +__tls_get_addr \+ [0-9a-f]+

Symbol table '\.dynsym' contains [0-9]+ entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +0: 0+ +0 NOTYPE +LOCAL +DEFAULT  UND *
 +1: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +7 *
 +2: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +8 *
 +3: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +9 *
 +4: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +[0-9]+ *
 +5: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +[0-9]+ *
 +6: [0-9a-f]+ +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
 +[0-9a-f]+: [0-9a-f]+ +0 NOTYPE  GLOBAL DEFAULT  UND __tls_get_addr
 +[0-9a-f]+: 0+00 +0 TLS +GLOBAL DEFAULT +8 sg1
#...
 +[0-9a-f]+: [0-9a-f]+ +0 FUNC +GLOBAL DEFAULT +7 fn1
#...
 +[0-9a-f]+: 0+04 +0 TLS +GLOBAL DEFAULT +8 sg2
#...

#pass

