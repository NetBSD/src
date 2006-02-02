#source: tlspic1.s
#source: tlspic2.s
#as: --64
#ld: -shared -melf_x86_64
#readelf: -WSsrl
#target: x86_64-*-*

There are 18 section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
  \[Nr\] Name +Type +Address +Off +Size +ES Flg Lk Inf Al
  \[ 0\] +NULL +0+ 0+ 0+ 00 +0 +0 +0
  \[ 1\] .hash +.*
  \[ 2\] .dynsym +.*
  \[ 3\] .dynstr +.*
  \[ 4\] .rela.dyn +.*
  \[ 5\] .rela.plt +.*
  \[ 6\] .plt +.*
  \[ 7\] .text +PROGBITS +0+1000 0+1000 0+1ac 00 +AX +0 +0 4096
  \[ 8\] .tdata +PROGBITS +0+1011ac 0+11ac 0+60 00 WAT +0 +0 +1
  \[ 9\] .tbss +NOBITS +0+10120c 0+120c 0+20 00 WAT +0 +0 +1
  \[10\] .dynamic +DYNAMIC +0+101210 0+1210 0+130 10 +WA +3 +0 +8
  \[11\] .got +PROGBITS +0+101340 0+1340 0+90 08 +WA +0 +0 +8
  \[12\] .got.plt +PROGBITS +0+1013d0 0+13d0 0+20 08 +WA +0 +0 +8
  \[13\] .data +.*
  \[14\] .bss +.*
  \[15\] .shstrtab +.*
  \[16\] .symtab +.*
  \[17\] .strtab +.*
Key to Flags:
.*
.*
.*

Elf file type is DYN \(Shared object file\)
Entry point 0x1000
There are 4 program headers, starting at offset [0-9]+

Program Headers:
  Type +Offset +VirtAddr +PhysAddr +FileSiz +MemSiz +Flg Align
  LOAD +0x0+ 0x0+ 0x0+ 0x[0-9a-f]+ 0x[0-9a-f]+ R E 0x100000
  LOAD +0x0+11ac 0x0+1011ac 0x0+1011ac 0x0+e54 0x0+e54 RW +0x100000
  DYNAMIC +0x0+1210 0x0+101210 0x0+101210 0x0+130 0x0+130 RW +0x8
  TLS +0x0+11ac 0x0+1011ac 0x0+1011ac 0x0+60 0x0+80 R +0x1

 Section to Segment mapping:
  Segment Sections...
   00 +.hash .dynsym .dynstr .rela.dyn .rela.plt .plt .text *
   01 +.tdata .dynamic .got .got.plt *
   02 +.dynamic *
   03 +.tdata .tbss *

Relocation section '.rela.dyn' at offset 0x[0-9a-f]+ contains 14 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+101340  0+10 R_X86_64_DTPMOD64 +0+
0+101350  0+12 R_X86_64_TPOFF64 +0+24
0+101358  0+12 R_X86_64_TPOFF64 +0+30
0+101360  0+10 R_X86_64_DTPMOD64 +0+
0+101370  0+10 R_X86_64_DTPMOD64 +0+
0+101380  0+12 R_X86_64_TPOFF64 +0+64
0+1013a0  0+12 R_X86_64_TPOFF64 +0+50
0+1013a8  0+12 R_X86_64_TPOFF64 +0+70
0+1013b8  0+10 R_X86_64_DTPMOD64 +0+
0+1013c8  0+12 R_X86_64_TPOFF64 +0+44
0+101388  0+a00000012 R_X86_64_TPOFF64 +0+10 sg5 \+ 0
0+101390  0+c00000010 R_X86_64_DTPMOD64 +0+ sg1 \+ 0
0+101398  0+c00000011 R_X86_64_DTPOFF64 +0+ sg1 \+ 0
0+1013b0  0+f00000012 R_X86_64_TPOFF64 +0+4 sg2 \+ 0

Relocation section '.rela.plt' at offset 0x[0-9a-f]+ contains 1 entries:
 +Offset +Info +Type +Symbol's Value  Symbol's Name \+ Addend
0+[0-9a-f]+  0+b00000007 R_X86_64_JUMP_SLOT +0+ __tls_get_addr \+ 0

Symbol table '.dynsym' contains 20 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +[0-9]+: 0+ +0 NOTYPE  LOCAL  DEFAULT  UND *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +7 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +8 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +9 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +13 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +14 *
 +[0-9]+: 0+1c +0 TLS +GLOBAL DEFAULT +8 sg8
 +[0-9]+: 0+101210 +0 OBJECT  GLOBAL DEFAULT  ABS _DYNAMIC
 +[0-9]+: 0+8 +0 TLS +GLOBAL DEFAULT +8 sg3
 +[0-9]+: 0+c +0 TLS +GLOBAL DEFAULT +8 sg4
 +[0-9]+: 0+10 +0 TLS +GLOBAL DEFAULT +8 sg5
 +[0-9]+: 0+ +0 NOTYPE  GLOBAL DEFAULT  UND __tls_get_addr
 +[0-9]+: 0+ +0 TLS +GLOBAL DEFAULT +8 sg1
 +[0-9]+: 0+1000 +0 FUNC +GLOBAL DEFAULT +7 fn1
 +[0-9]+: [0-9a-f]+ +0 NOTYPE  GLOBAL DEFAULT  ABS __bss_start
 +[0-9]+: 0+4 +0 TLS +GLOBAL DEFAULT +8 sg2
 +[0-9]+: 0+14 +0 TLS +GLOBAL DEFAULT +8 sg6
 +[0-9]+: 0+18 +0 TLS +GLOBAL DEFAULT +8 sg7
 +[0-9]+: [0-9a-f]+ +0 NOTYPE  GLOBAL DEFAULT  ABS _edata
 +[0-9]+: [0-9a-f]+ +0 NOTYPE  GLOBAL DEFAULT  ABS _end

Symbol table '.symtab' contains 57 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +[0-9]+: 0+ +0 NOTYPE  LOCAL  DEFAULT  UND *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +1 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +2 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +3 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +4 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +5 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +6 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +7 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +8 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +9 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +10 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +11 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +12 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +13 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +14 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +15 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +16 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL  DEFAULT +17 *
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
 +[0-9]+: 0+1013d0 +0 OBJECT  LOCAL  HIDDEN  ABS _GLOBAL_OFFSET_TABLE_
 +[0-9]+: 0+44 +0 TLS +LOCAL  HIDDEN +8 sh2
 +[0-9]+: 0+54 +0 TLS +LOCAL  HIDDEN +8 sh6
 +[0-9]+: 0+1c +0 TLS +GLOBAL DEFAULT +8 sg8
 +[0-9]+: 0+101210 +0 OBJECT  GLOBAL DEFAULT  ABS _DYNAMIC
 +[0-9]+: 0+8 +0 TLS +GLOBAL DEFAULT +8 sg3
 +[0-9]+: 0+c +0 TLS +GLOBAL DEFAULT +8 sg4
 +[0-9]+: 0+10 +0 TLS +GLOBAL DEFAULT +8 sg5
 +[0-9]+: 0+ +0 NOTYPE  GLOBAL DEFAULT  UND __tls_get_addr
 +[0-9]+: 0+ +0 TLS +GLOBAL DEFAULT +8 sg1
 +[0-9]+: 0+1000 +0 FUNC +GLOBAL DEFAULT +7 fn1
 +[0-9]+: [0-9a-f]+ +0 NOTYPE  GLOBAL DEFAULT  ABS __bss_start
 +[0-9]+: 0+4 +0 TLS +GLOBAL DEFAULT +8 sg2
 +[0-9]+: 0+14 +0 TLS +GLOBAL DEFAULT +8 sg6
 +[0-9]+: 0+18 +0 TLS +GLOBAL DEFAULT +8 sg7
 +[0-9]+: [0-9a-f]+ +0 NOTYPE  GLOBAL DEFAULT  ABS _edata
 +[0-9]+: [0-9a-f]+ +0 NOTYPE  GLOBAL DEFAULT  ABS _end
