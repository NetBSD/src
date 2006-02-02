#source: tlssunpic32.s
#source: tlspic.s
#as: --32 -K PIC
#ld: -shared -melf32_sparc
#readelf: -WSsrl
#target: sparc*-*-*

There are 17 section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
 +\[Nr\] Name +Type +Addr +Off +Size +ES Flg Lk Inf Al
 +\[ 0\] +NULL +0+ 0+ 0+ 0+ +0 +0 +0
 +\[ 1\] .hash +.*
 +\[ 2\] .dynsym +.*
 +\[ 3\] .dynstr +.*
 +\[ 4\] .rela.dyn +.*
 +\[ 5\] .rela.plt +.*
 +\[ 6\] .text +PROGBITS +0+1000 0+1000 0+1000 0+ +AX +0 +0 4096
 +\[ 7\] .tdata +PROGBITS +0+12000 0+2000 0+60 0+ WAT +0 +0 +4
 +\[ 8\] .tbss +NOBITS +0+12060 0+2060 0+20 0+ WAT +0 +0 +4
 +\[ 9\] .dynamic +DYNAMIC +0+12060 0+2060 0+98 08 +WA +3 +0 +4
 +\[10\] .got +PROGBITS +0+120f8 0+20f8 0+4c 04 +WA +0 +0 +4
 +\[11\] .plt +.*
 +\[12\] .data +PROGBITS +0+13000 0+3000 0+ 0+ +WA +0 +0 4096
 +\[13\] .bss +.*
 +\[14\] .shstrtab +.*
 +\[15\] .symtab +.*
 +\[16\] .strtab +.*
#...

Elf file type is DYN \(Shared object file\)
Entry point 0x1000
There are 4 program headers, starting at offset [0-9]+

Program Headers:
 +Type +Offset +VirtAddr +PhysAddr +FileSiz MemSiz +Flg Align
 +LOAD +0x0+ 0x0+ 0x0+ 0x0+2000 0x0+2000 R E 0x10000
 +LOAD +0x0+2000 0x0+12000 0x0+12000 0x0+1000 0x0+1000 RWE 0x10000
 +DYNAMIC +0x0+2060 0x0+12060 0x0+12060 0x0+98 0x0+98 RW +0x4
 +TLS +0x0+2000 0x0+12000 0x0+12000 0x0+60 0x0+80 R +0x4
#...

Relocation section '.rela.dyn' at offset 0x[0-9a-f]+ contains 14 entries:
 Offset +Info +Type +Sym. Value +Symbol's Name \+ Addend
0+120fc +0+4a R_SPARC_TLS_DTPMOD32 +0+
0+12104 +0+4e R_SPARC_TLS_TPOFF32 +0+24
0+12108 +0+4e R_SPARC_TLS_TPOFF32 +0+30
0+1210c +0+4a R_SPARC_TLS_DTPMOD32 +0+
0+12114 +0+4a R_SPARC_TLS_DTPMOD32 +0+
0+1211c +0+4e R_SPARC_TLS_TPOFF32 +0+64
0+1212c +0+4e R_SPARC_TLS_TPOFF32 +0+50
0+12130 +0+4e R_SPARC_TLS_TPOFF32 +0+70
0+12138 +0+4a R_SPARC_TLS_DTPMOD32 +0+
0+12140 +0+4e R_SPARC_TLS_TPOFF32 +0+44
0+12120 +0+b4e R_SPARC_TLS_TPOFF32 +0+10 +sg5 \+ 0
0+12124 +0+e4a R_SPARC_TLS_DTPMOD32 +0+ +sg1 \+ 0
0+12128 +0+e4c R_SPARC_TLS_DTPOFF32 +0+ +sg1 \+ 0
0+12134 +0+114e R_SPARC_TLS_TPOFF32 +0+4 +sg2 \+ 0

Relocation section '.rela.plt' at offset 0x[0-9a-f]+ contains 1 entries:
 Offset +Info +Type +Sym. Value +Symbol's Name \+ Addend
0+12174 +0+d15 R_SPARC_JMP_SLOT +0+ +__tls_get_addr \+ 0

Symbol table '.dynsym' contains 22 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +0: 0+ +0 NOTYPE +LOCAL +DEFAULT +UND *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +6 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +7 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +8 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +10 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +12 *
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +13 *
 +[0-9]+: 0+1c +0 TLS +GLOBAL DEFAULT +7 sg8
 +[0-9]+: 0+12060 +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
 +[0-9]+: 0+8 +0 TLS +GLOBAL DEFAULT +7 sg3
 +[0-9]+: 0+c +0 TLS +GLOBAL DEFAULT +7 sg4
 +[0-9]+: 0+10 +0 TLS +GLOBAL DEFAULT +7 sg5
 +[0-9]+: [0-9a-f]+ +0 OBJECT +GLOBAL DEFAULT +ABS _PROCEDURE_LINKAGE_TABLE_
 +[0-9]+: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND __tls_get_addr
 +[0-9]+: 0+ +0 TLS +GLOBAL DEFAULT +7 sg1
 +[0-9]+: 0+1008 +0 FUNC +GLOBAL DEFAULT +6 fn1
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
 +[0-9]+: 0+4 +0 TLS +GLOBAL DEFAULT +7 sg2
 +[0-9]+: 0+14 +0 TLS +GLOBAL DEFAULT +7 sg6
 +[0-9]+: 0+18 +0 TLS +GLOBAL DEFAULT +7 sg7
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS _end

Symbol table '.symtab' contains 57 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +0: 0+ +0 NOTYPE +LOCAL +DEFAULT +UND *
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
 +[0-9]+: 0+20 +0 TLS +LOCAL +DEFAULT +7 sl1
 +[0-9]+: 0+24 +0 TLS +LOCAL +DEFAULT +7 sl2
 +[0-9]+: 0+28 +0 TLS +LOCAL +DEFAULT +7 sl3
 +[0-9]+: 0+2c +0 TLS +LOCAL +DEFAULT +7 sl4
 +[0-9]+: 0+30 +0 TLS +LOCAL +DEFAULT +7 sl5
 +[0-9]+: 0+34 +0 TLS +LOCAL +DEFAULT +7 sl6
 +[0-9]+: 0+38 +0 TLS +LOCAL +DEFAULT +7 sl7
 +[0-9]+: 0+3c +0 TLS +LOCAL +DEFAULT +7 sl8
 +[0-9]+: 0+60 +0 TLS +LOCAL +HIDDEN +8 sH1
 +[0-9]+: 0+48 +0 TLS +LOCAL +HIDDEN +7 sh3
 +[0-9]+: 0+64 +0 TLS +LOCAL +HIDDEN +8 sH2
 +[0-9]+: 0+78 +0 TLS +LOCAL +HIDDEN +8 sH7
 +[0-9]+: 0+58 +0 TLS +LOCAL +HIDDEN +7 sh7
 +[0-9]+: 0+5c +0 TLS +LOCAL +HIDDEN +7 sh8
 +[0-9]+: 0+6c +0 TLS +LOCAL +HIDDEN +8 sH4
 +[0-9]+: 0+4c +0 TLS +LOCAL +HIDDEN +7 sh4
 +[0-9]+: 0+68 +0 TLS +LOCAL +HIDDEN +8 sH3
 +[0-9]+: 0+50 +0 TLS +LOCAL +HIDDEN +7 sh5
 +[0-9]+: 0+70 +0 TLS +LOCAL +HIDDEN +8 sH5
 +[0-9]+: 0+74 +0 TLS +LOCAL +HIDDEN +8 sH6
 +[0-9]+: 0+7c +0 TLS +LOCAL +HIDDEN +8 sH8
 +[0-9]+: 0+40 +0 TLS +LOCAL +HIDDEN +7 sh1
 +[0-9]+: 0+120f8 +0 OBJECT +LOCAL  HIDDEN +ABS _GLOBAL_OFFSET_TABLE_
 +[0-9]+: 0+44 +0 TLS +LOCAL +HIDDEN +7 sh2
 +[0-9]+: 0+54 +0 TLS +LOCAL +HIDDEN +7 sh6
 +[0-9]+: 0+1c +0 TLS +GLOBAL DEFAULT +7 sg8
 +[0-9]+: 0+12060 +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
 +[0-9]+: 0+8 +0 TLS +GLOBAL DEFAULT +7 sg3
 +[0-9]+: 0+c +0 TLS +GLOBAL DEFAULT +7 sg4
 +[0-9]+: 0+10 +0 TLS +GLOBAL DEFAULT +7 sg5
 +[0-9]+: [0-9a-f]+ +0 OBJECT +GLOBAL DEFAULT +ABS _PROCEDURE_LINKAGE_TABLE_
 +[0-9]+: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND __tls_get_addr
 +[0-9]+: 0+ +0 TLS +GLOBAL DEFAULT +7 sg1
 +[0-9]+: 0+1008 +0 FUNC +GLOBAL DEFAULT +6 fn1
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
 +[0-9]+: 0+4 +0 TLS +GLOBAL DEFAULT +7 sg2
 +[0-9]+: 0+14 +0 TLS +GLOBAL DEFAULT +7 sg6
 +[0-9]+: 0+18 +0 TLS +GLOBAL DEFAULT +7 sg7
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS _end
