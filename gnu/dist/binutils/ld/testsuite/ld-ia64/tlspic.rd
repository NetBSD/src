#source: tlspic1.s
#source: tlspic2.s
#as:
#ld: -shared
#readelf: -WSsrl
#target: ia64-*-*

There are 21 section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
  \[Nr\] Name +Type +Address +Off +Size +ES Flg Lk Inf Al
  \[ 0\] +NULL +0+ 0+ 0+ 00 +0 +0 +0
  \[ 1\] .hash +.*
  \[ 2\] .dynsym +.*
  \[ 3\] .dynstr +.*
  \[ 4\] .rela.dyn +.*
  \[ 5\] .rela.IA_64.pltof +.*
  \[ 6\] .plt +.*
  \[ 7\] .text +PROGBITS +0+1000 0+1000 0+110 00 +AX +0 +0 4096
  \[ 8\] .IA_64.unwind_inf +.*
  \[ 9\] .IA_64.unwind +.*
  \[10\] .tdata +PROGBITS +0+11[0-9a-f]+ 0+1[0-9a-f]+ 0+60 00 WAT +0 +0 +4
  \[11\] .tbss +NOBITS +0+11[0-9a-f]+ 0+1[0-9a-f]+ 0+20 00 WAT +0 +0 +1
  \[12\] .dynamic +DYNAMIC +0+11[0-9a-f]+ 0+1[0-9a-f]+ 0+140 10 +WA +3 +0 +8
  \[13\] .data +.*
  \[14\] .got +PROGBITS +0+12000 0+2000 0+50 00 WAp +0 +0 +8
  \[15\] .IA_64.pltoff +.*
  \[16\] .sbss +.*
  \[17\] .bss +.*
  \[18\] .shstrtab +.*
  \[19\] .symtab +.*
  \[20\] .strtab +.*
Key to Flags:
#...

Elf file type is DYN \(Shared object file\)
Entry point 0x1000
There are 5 program headers, starting at offset [0-9]+

Program Headers:
  Type +Offset +VirtAddr +PhysAddr +FileSiz +MemSiz +Flg Align
  LOAD +0x0+ 0x0+ 0x0+ 0x0+1[0-9a-f]+ 0x0+1[0-9a-f]+ R E 0x10000
  LOAD +0x0+1[0-9a-f]+ 0x0+11[0-9a-f]+ 0x0+11[0-9a-f]+ 0x0+0[0-9a-f]+ 0x0+0[0-9a-f]+ RW +0x10000
  DYNAMIC +0x0+1[0-9a-f]+ 0x0+11[0-9a-f]+ 0x0+11[0-9a-f]+ 0x0+140 0x0+140 RW +0x8
  TLS +0x0+1[0-9a-f]+ 0x0+11[0-9a-f]+ 0x0+11[0-9a-f]+ 0x0+60 0x0+80 R +0x4
  IA_64_UNWIND +0x0+1[0-9a-f]+ 0x0+1[0-9a-f]+ 0x0+1[0-9a-f]+ 0x0+18 0x0+18 R +0x8
#...

Relocation section '.rela.dyn' at offset 0x[0-9a-f]+ contains 6 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+12018 +0+f000000a7 R_IA64_DTPMOD64LSB +0+ sg1 \+ 0
0+12020 +0+f000000b7 R_IA64_DTPREL64LSB +0+ sg1 \+ 0
0+12028 +0+1200000097 R_IA64_TPREL64LSB +0+4 sg2 \+ 0
0+12030 +0+a7 R_IA64_DTPMOD64LSB +0+
0+12038 +0+97 R_IA64_TPREL64LSB +0+44
0+12048 +0+97 R_IA64_TPREL64LSB +0+24

Relocation section '.rela.IA_64.pltoff' at offset 0x[0-9a-f]+ contains 1 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+[0-9a-f]+ +0+e00000081 R_IA64_IPLTLSB +0+ __tls_get_addr \+ 0

Symbol table '.dynsym' contains 23 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +[0-9]+: 0+ +0 NOTYPE +LOCAL +DEFAULT +UND *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +7 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +8 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +9 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +10 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +11 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +13 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +15 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +17 *
 +[0-9]+: 0+1c +0 TLS +GLOBAL DEFAULT +10 sg8
 +[0-9]+: 0+11[0-9a-f]+ +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
 +[0-9]+: 0+8 +0 TLS +GLOBAL DEFAULT +10 sg3
 +[0-9]+: 0+c +0 TLS +GLOBAL DEFAULT +10 sg4
 +[0-9]+: 0+10 +0 TLS +GLOBAL DEFAULT +10 sg5
 +[0-9]+: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND __tls_get_addr
 +[0-9]+: 0+ +0 TLS +GLOBAL DEFAULT +10 sg1
 +[0-9]+: 0+1000 +272 FUNC +GLOBAL DEFAULT +7 fn1
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
 +[0-9]+: 0+4 +0 TLS +GLOBAL DEFAULT +10 sg2
 +[0-9]+: 0+14 +0 TLS +GLOBAL DEFAULT +10 sg6
 +[0-9]+: 0+18 +0 TLS +GLOBAL DEFAULT +10 sg7
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS _end

Symbol table '.symtab' contains 60 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +[0-9]+: 0+ +0 NOTYPE +LOCAL +DEFAULT +UND *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +1 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +2 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +3 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +4 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +5 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +6 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +7 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +8 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +9 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +10 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +11 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +12 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +13 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +14 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +15 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +16 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +17 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +18 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +19 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +20 *
 +[0-9]+: 0+20 +0 TLS +LOCAL +DEFAULT +10 sl1
 +[0-9]+: 0+24 +0 TLS +LOCAL +DEFAULT +10 sl2
 +[0-9]+: 0+28 +0 TLS +LOCAL +DEFAULT +10 sl3
 +[0-9]+: 0+2c +0 TLS +LOCAL +DEFAULT +10 sl4
 +[0-9]+: 0+30 +0 TLS +LOCAL +DEFAULT +10 sl5
 +[0-9]+: 0+34 +0 TLS +LOCAL +DEFAULT +10 sl6
 +[0-9]+: 0+38 +0 TLS +LOCAL +DEFAULT +10 sl7
 +[0-9]+: 0+3c +0 TLS +LOCAL +DEFAULT +10 sl8
 +[0-9]+: 0+60 +0 TLS +LOCAL +HIDDEN +11 sH1
 +[0-9]+: 0+48 +0 TLS +LOCAL +HIDDEN +10 sh3
 +[0-9]+: 0+64 +0 TLS +LOCAL +HIDDEN +11 sH2
 +[0-9]+: 0+78 +0 TLS +LOCAL +HIDDEN +11 sH7
 +[0-9]+: 0+58 +0 TLS +LOCAL +HIDDEN +10 sh7
 +[0-9]+: 0+5c +0 TLS +LOCAL +HIDDEN +10 sh8
 +[0-9]+: 0+6c +0 TLS +LOCAL +HIDDEN +11 sH4
 +[0-9]+: 0+4c +0 TLS +LOCAL +HIDDEN +10 sh4
 +[0-9]+: 0+68 +0 TLS +LOCAL +HIDDEN +11 sH3
 +[0-9]+: 0+50 +0 TLS +LOCAL +HIDDEN +10 sh5
 +[0-9]+: 0+70 +0 TLS +LOCAL +HIDDEN +11 sH5
 +[0-9]+: 0+74 +0 TLS +LOCAL +HIDDEN +11 sH6
 +[0-9]+: 0+7c +0 TLS +LOCAL +HIDDEN +11 sH8
 +[0-9]+: 0+40 +0 TLS +LOCAL +HIDDEN +10 sh1
 +[0-9]+: 0+12000 +0 OBJECT +LOCAL +HIDDEN +ABS _GLOBAL_OFFSET_TABLE_
 +[0-9]+: 0+44 +0 TLS +LOCAL +HIDDEN +10 sh2
 +[0-9]+: 0+54 +0 TLS +LOCAL +HIDDEN +10 sh6
 +[0-9]+: 0+1c +0 TLS +GLOBAL DEFAULT +10 sg8
 +[0-9]+: 0+11[0-9a-f]+ +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
 +[0-9]+: 0+8 +0 TLS +GLOBAL DEFAULT +10 sg3
 +[0-9]+: 0+c +0 TLS +GLOBAL DEFAULT +10 sg4
 +[0-9]+: 0+10 +0 TLS +GLOBAL DEFAULT +10 sg5
 +[0-9]+: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND __tls_get_addr
 +[0-9]+: 0+ +0 TLS +GLOBAL DEFAULT +10 sg1
 +[0-9]+: 0+1000 +272 FUNC +GLOBAL DEFAULT +7 fn1
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
 +[0-9]+: 0+4 +0 TLS +GLOBAL DEFAULT +10 sg2
 +[0-9]+: 0+14 +0 TLS +GLOBAL DEFAULT +10 sg6
 +[0-9]+: 0+18 +0 TLS +GLOBAL DEFAULT +10 sg7
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS _end
