#source: tlspic1.s
#source: tlspic2.s
#as: -m31
#ld: -shared -melf_s390
#readelf: -Ssrl
#target: s390-*-*

There are 17 section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
  \[Nr\] Name +Type +Addr +Off +Size +ES Flg Lk Inf Al
  \[ 0\] +NULL +0+ 0+ 0+ 00 +0 +0  0
  \[ 1\] .hash +.*
  \[ 2\] .dynsym +.*
  \[ 3\] .dynstr +.*
  \[ 4\] .rela.dyn +.*
  \[ 5\] .rela.plt +.*
  \[ 6\] .plt +.*
  \[ 7\] .text +PROGBITS +.*
  \[ 8\] .tdata +PROGBITS +0+15c0 0+5c0 0+60 00 WAT  0 +0 32
  \[ 9\] .tbss +NOBITS +0+1620 0+620 0+20 00 WAT  0 +0  1
  \[10\] .dynamic +DYNAMIC +0+1620 0+620 0+98 08  WA  3 +0  4
  \[11\] .got +PROGBITS +0+16b8 0+6b8 0+58 04  WA  0 +0  4
  \[12\] .data +.*
  \[13\] .bss +.*
  \[14\] .shstrtab +.*
  \[15\] .symtab +.*
  \[16\] .strtab +.*
Key to Flags:
.*
.*
.*

Elf file type is DYN \(Shared object file\)
Entry point 0x[0-9a-f]+
There are 4 program headers, starting at offset [0-9]+

Program Headers:
  Type +Offset +VirtAddr +PhysAddr +FileSiz MemSiz  Flg Align
  LOAD +0x0+ 0x0+ 0x0+ 0x[0-9a-f]+ 0x[0-9a-f]+ R E 0x1000
  LOAD +0x0+5c0 0x0+15c0 0x0+15c0 0x00150 0x00150 RW  0x1000
  DYNAMIC +0x0+620 0x0+1620 0x0+1620 0x0+98 0x0+98 RW  0x4
  TLS +0x0+5c0 0x0+15c0 0x0+15c0 0x0+60 0x0+80 R +0x20

 Section to Segment mapping:
  Segment Sections...
 +00 +.hash .dynsym .dynstr .rela.dyn .rela.plt .plt .text 
 +01 +.tdata .dynamic .got 
 +02 +.dynamic 
 +03 +.tdata .tbss 

Relocation section '.rela.dyn' at offset 0x[0-9a-f]+ contains 14 entries:
 Offset +Info +Type +Sym.Value  Sym. Name \+ Addend
[0-9a-f]+  0+36 R_390_TLS_DTPMOD +0+
[0-9a-f]+  0+38 R_390_TLS_TPOFF +0+24
[0-9a-f]+  0+38 R_390_TLS_TPOFF +0+30
[0-9a-f]+  0+36 R_390_TLS_DTPMOD +0+
[0-9a-f]+  0+36 R_390_TLS_DTPMOD +0+
[0-9a-f]+  0+38 R_390_TLS_TPOFF +0+64
[0-9a-f]+  0+38 R_390_TLS_TPOFF +0+50
[0-9a-f]+  0+38 R_390_TLS_TPOFF +0+70
[0-9a-f]+  0+36 R_390_TLS_DTPMOD +0+
[0-9a-f]+  0+38 R_390_TLS_TPOFF +0+44
[0-9a-f]+  0+a38 R_390_TLS_TPOFF +0+10 +sg5 \+ 0
[0-9a-f]+  0+c36 R_390_TLS_DTPMOD  0+ +sg1 \+ 0
[0-9a-f]+  0+c37 R_390_TLS_DTPOFF  0+ +sg1 \+ 0
[0-9a-f]+  0+f38 R_390_TLS_TPOFF +0+4 +sg2 \+ 0

Relocation section '.rela.plt' at offset 0x[0-9a-f]+ contains 1 entries:
 Offset +Info +Type +Sym.Value  Sym. Name \+ Addend
[0-9a-f]+  0+b0b R_390_JMP_SLOT +0+ +__tls_get_offset \+ 0

Symbol table '.dynsym' contains 20 entries:
 +Num: +Value  Size Type +Bind +Vis +Ndx Name
 +[0-9]+: 0+ +0 NOTYPE  LOCAL  DEFAULT  UND 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +7 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +8 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +9 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +12 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +13 
 +[0-9]+: 0+1c +0 TLS +GLOBAL DEFAULT +8 sg8
 +[0-9]+: [0-9a-f]+ +0 OBJECT  GLOBAL DEFAULT  ABS _DYNAMIC
 +[0-9]+: 0+8 +0 TLS +GLOBAL DEFAULT +8 sg3
 +[0-9]+: 0+c +0 TLS +GLOBAL DEFAULT +8 sg4
 +[0-9]+: 0+10 +0 TLS +GLOBAL DEFAULT +8 sg5
 +[0-9]+: 0+ +0 NOTYPE  GLOBAL DEFAULT  UND __tls_get_offset
 +[0-9]+: 0+ +0 TLS +GLOBAL DEFAULT +8 sg1
 +[0-9]+: [0-9a-f]+ +0 FUNC +GLOBAL DEFAULT +7 fn1
 +[0-9]+: [0-9a-f]+ +0 NOTYPE  GLOBAL DEFAULT  ABS __bss_start
 +[0-9]+: 0+4 +0 TLS +GLOBAL DEFAULT +8 sg2
 +[0-9]+: 0+14 +0 TLS +GLOBAL DEFAULT +8 sg6
 +[0-9]+: 0+18 +0 TLS +GLOBAL DEFAULT +8 sg7
 +[0-9]+: [0-9a-f]+ +0 NOTYPE  GLOBAL DEFAULT  ABS _edata
 +[0-9]+: [0-9a-f]+ +0 NOTYPE  GLOBAL DEFAULT  ABS _end

Symbol table '.symtab' contains 56 entries:
 +Num: +Value  Size Type +Bind +Vis +Ndx Name
 +[0-9]+: 0+ +0 NOTYPE  LOCAL  DEFAULT  UND 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +1 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +2 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +3 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +4 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +5 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +6 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +7 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +8 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +9 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +10 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +11 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +12 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +13 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +14 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +15 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +16 
 +[0-9]+: 0+20 +0 TLS +LOCAL  DEFAULT +8 sl1
 +[0-9]+: 0+24 +0 TLS +LOCAL  DEFAULT +8 sl2
 +[0-9]+: 0+28 +0 TLS +LOCAL  DEFAULT +8 sl3
 +[0-9]+: 0+2c +0 TLS +LOCAL  DEFAULT +8 sl4
 +[0-9]+: 0+30 +0 TLS +LOCAL  DEFAULT +8 sl5
 +[0-9]+: 0+34 +0 TLS +LOCAL  DEFAULT +8 sl6
 +[0-9]+: 0+38 +0 TLS +LOCAL  DEFAULT +8 sl7
 +[0-9]+: 0+3c +0 TLS +LOCAL  DEFAULT +8 sl8
 +[0-9]+: 0+60 +0 TLS +LOCAL  HIDDEN +9 sH1
 +[0-9]+: 0+48 +0 TLS +LOCAL  HIDDEN +8 sh3
 +[0-9]+: 0+64 +0 TLS +LOCAL  HIDDEN +9 sH2
 +[0-9]+: 0+78 +0 TLS +LOCAL  HIDDEN +9 sH7
 +[0-9]+: 0+58 +0 TLS +LOCAL  HIDDEN +8 sh7
 +[0-9]+: 0+5c +0 TLS +LOCAL  HIDDEN +8 sh8
 +[0-9]+: 0+6c +0 TLS +LOCAL  HIDDEN +9 sH4
 +[0-9]+: 0+4c +0 TLS +LOCAL  HIDDEN +8 sh4
 +[0-9]+: 0+68 +0 TLS +LOCAL  HIDDEN +9 sH3
 +[0-9]+: 0+50 +0 TLS +LOCAL  HIDDEN +8 sh5
 +[0-9]+: 0+70 +0 TLS +LOCAL  HIDDEN +9 sH5
 +[0-9]+: 0+74 +0 TLS +LOCAL  HIDDEN +9 sH6
 +[0-9]+: 0+7c +0 TLS +LOCAL  HIDDEN +9 sH8
 +[0-9]+: 0+40 +0 TLS +LOCAL  HIDDEN +8 sh1
 +[0-9]+: [0-9a-f]+ +0 OBJECT  LOCAL  HIDDEN  ABS _GLOBAL_OFFSET_TABLE_
 +[0-9]+: 0+44 +0 TLS +LOCAL  HIDDEN +8 sh2
 +[0-9]+: 0+54 +0 TLS +LOCAL  HIDDEN +8 sh6
 +[0-9]+: 0+1c +0 TLS +GLOBAL DEFAULT +8 sg8
 +[0-9]+: [0-9a-f]+ +0 OBJECT  GLOBAL DEFAULT  ABS _DYNAMIC
 +[0-9]+: 0+8 +0 TLS +GLOBAL DEFAULT +8 sg3
 +[0-9]+: 0+c +0 TLS +GLOBAL DEFAULT +8 sg4
 +[0-9]+: 0+10 +0 TLS +GLOBAL DEFAULT +8 sg5
 +[0-9]+: 0+ +0 NOTYPE  GLOBAL DEFAULT  UND __tls_get_offset
 +[0-9]+: 0+ +0 TLS +GLOBAL DEFAULT +8 sg1
 +[0-9]+: [0-9a-f]+ +0 FUNC +GLOBAL DEFAULT +7 fn1
 +[0-9]+: [0-9a-f]+ +0 NOTYPE  GLOBAL DEFAULT  ABS __bss_start
 +[0-9]+: 0+4 +0 TLS +GLOBAL DEFAULT +8 sg2
 +[0-9]+: 0+14 +0 TLS +GLOBAL DEFAULT +8 sg6
 +[0-9]+: 0+18 +0 TLS +GLOBAL DEFAULT +8 sg7
 +[0-9]+: [0-9a-f]+ +0 NOTYPE  GLOBAL DEFAULT  ABS _edata
 +[0-9]+: [0-9a-f]+ +0 NOTYPE  GLOBAL DEFAULT  ABS _end
