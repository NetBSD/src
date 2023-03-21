#source: tlspic1.s
#source: tlspic2.s
#as:
#ld: -shared
#readelf: -WSsrl
#target: ia64-*-*

There are [0-9]+ section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
 +\[Nr\] Name +Type +Address +Off +Size +ES Flg Lk Inf Al
 +\[[ 0-9]+\] +NULL +0+ 0+ 0+ 00 +0 +0 +0
 +\[[ 0-9]+\] .hash +.*
 +\[[ 0-9]+\] .dynsym +.*
 +\[[ 0-9]+\] .dynstr +.*
 +\[[ 0-9]+\] .rela.dyn +.*
 +\[[ 0-9]+\] .rela.IA_64.pltoff +.*
 +\[[ 0-9]+\] .plt +.*
 +\[[ 0-9]+\] .text +PROGBITS +0+1000 0+1000 0+110 00 +AX +0 +0 4096
 +\[[ 0-9]+\] .IA_64.unwind_info +.*
 +\[[ 0-9]+\] .IA_64.unwind +.*
 +\[[ 0-9]+\] .tdata +PROGBITS +0+11[0-9a-f]+ 0+1[0-9a-f]+ 0+60 00 WAT +0 +0 +4
 +\[[ 0-9]+\] .tbss +NOBITS +0+11[0-9a-f]+ 0+1[0-9a-f]+ 0+20 00 WAT +0 +0 +1
 +\[[ 0-9]+\] .dynamic +DYNAMIC +0+11[0-9a-f]+ 0+1[0-9a-f]+ 0+140 10 +WA +3 +0 +8
 +\[[ 0-9]+\] .got +PROGBITS +0+112d8 0+12d8 0+50 00 WAp +0 +0 +8
 +\[[ 0-9]+\] .IA_64.pltoff +.*
 +\[[ 0-9]+\] .symtab +.*
 +\[[ 0-9]+\] .strtab +.*
 +\[[ 0-9]+\] .shstrtab +.*
Key to Flags:
#...

Elf file type is DYN \(Shared object file\)
Entry point 0x1000
There are [0-9]+ program headers, starting at offset [0-9]+

Program Headers:
 +Type +Offset +VirtAddr +PhysAddr +FileSiz +MemSiz +Flg Align
 +LOAD +0x0+ 0x0+ 0x0+ 0x0+1[0-9a-f]+ 0x0+1[0-9a-f]+ R E 0x10000
 +LOAD +0x0+1[0-9a-f]+ 0x0+11[0-9a-f]+ 0x0+11[0-9a-f]+ 0x0+0[0-9a-f]+ 0x0+0[0-9a-f]+ RW +0x10000
 +DYNAMIC +0x0+1[0-9a-f]+ 0x0+11[0-9a-f]+ 0x0+11[0-9a-f]+ 0x0+140 0x0+140 RW +0x8
 +TLS +0x0+1[0-9a-f]+ 0x0+11[0-9a-f]+ 0x0+11[0-9a-f]+ 0x0+60 0x0+80 R +0x4
 +IA_64_UNWIND +0x0+1[0-9a-f]+ 0x0+1[0-9a-f]+ 0x0+1[0-9a-f]+ 0x0+18 0x0+18 R +0x8
#...

Relocation section '.rela.dyn' at offset 0x[0-9a-f]+ contains 6 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
[0-9a-f ]+R_IA64_DTPMOD64LSB +0+ sg1 \+ 0
[0-9a-f ]+R_IA64_DTPREL64LSB +0+ sg1 \+ 0
[0-9a-f ]+R_IA64_TPREL64LSB +0+4 sg2 \+ 0
[0-9a-f ]+R_IA64_DTPMOD64LSB +0
[0-9a-f ]+R_IA64_TPREL64LSB +44
[0-9a-f ]+R_IA64_TPREL64LSB +24

Relocation section '.rela.IA_64.pltoff' at offset 0x[0-9a-f]+ contains 1 entry:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
[0-9a-f ]+R_IA64_IPLTLSB +0+ __tls_get_addr \+ 0

Symbol table '\.dynsym' contains [0-9]+ entries:
 +Num: +Value +Size +Type +Bind +Vis +Ndx +Name
.* NOTYPE +LOCAL +DEFAULT +UND *
.* TLS +GLOBAL +DEFAULT +10 sg8
.* TLS +GLOBAL +DEFAULT +10 sg3
.* TLS +GLOBAL +DEFAULT +10 sg4
.* TLS +GLOBAL +DEFAULT +10 sg5
.* NOTYPE +GLOBAL +DEFAULT +UND __tls_get_addr
.* TLS +GLOBAL +DEFAULT +10 sg1
.* FUNC +GLOBAL +DEFAULT +7 fn1
.* TLS +GLOBAL +DEFAULT +10 sg2
.* TLS +GLOBAL +DEFAULT +10 sg6
.* TLS +GLOBAL +DEFAULT +10 sg7

Symbol table '\.symtab' contains [0-9]+ entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
.* NOTYPE +LOCAL +DEFAULT +UND *
.* SECTION +LOCAL +DEFAULT +1 *
.* SECTION +LOCAL +DEFAULT +2 *
.* SECTION +LOCAL +DEFAULT +3 *
.* SECTION +LOCAL +DEFAULT +4 *
.* SECTION +LOCAL +DEFAULT +5 *
.* SECTION +LOCAL +DEFAULT +6 *
.* SECTION +LOCAL +DEFAULT +7 *
.* SECTION +LOCAL +DEFAULT +8 *
.* SECTION +LOCAL +DEFAULT +9 *
.* SECTION +LOCAL +DEFAULT +10 *
.* SECTION +LOCAL +DEFAULT +11 *
.* SECTION +LOCAL +DEFAULT +12 *
.* SECTION +LOCAL +DEFAULT +13 *
.* SECTION +LOCAL +DEFAULT +14 *
.* FILE +LOCAL +DEFAULT +ABS .*
.* TLS +LOCAL +DEFAULT +10 sl1
.* TLS +LOCAL +DEFAULT +10 sl2
.* TLS +LOCAL +DEFAULT +10 sl3
.* TLS +LOCAL +DEFAULT +10 sl4
.* TLS +LOCAL +DEFAULT +10 sl5
.* TLS +LOCAL +DEFAULT +10 sl6
.* TLS +LOCAL +DEFAULT +10 sl7
.* TLS +LOCAL +DEFAULT +10 sl8
.* FILE +LOCAL +DEFAULT +ABS 
.* TLS +LOCAL +DEFAULT +11 sH1
.* OBJECT +LOCAL +DEFAULT +ABS _DYNAMIC
.* TLS +LOCAL +DEFAULT +10 sh3
.* TLS +LOCAL +DEFAULT +11 sH2
.* TLS +LOCAL +DEFAULT +11 sH7
.* TLS +LOCAL +DEFAULT +10 sh7
.* TLS +LOCAL +DEFAULT +10 sh8
.* TLS +LOCAL +DEFAULT +11 sH4
.* TLS +LOCAL +DEFAULT +10 sh4
.* TLS +LOCAL +DEFAULT +11 sH3
.* TLS +LOCAL +DEFAULT +10 sh5
.* TLS +LOCAL +DEFAULT +11 sH5
.* TLS +LOCAL +DEFAULT +11 sH6
.* TLS +LOCAL +DEFAULT +11 sH8
.* TLS +LOCAL +DEFAULT +10 sh1
.* OBJECT +LOCAL +DEFAULT +ABS _GLOBAL_OFFSET_TABLE_
.* TLS +LOCAL +DEFAULT +10 sh2
.* TLS +LOCAL +DEFAULT +10 sh6
.* TLS +GLOBAL +DEFAULT +10 sg8
.* TLS +GLOBAL +DEFAULT +10 sg3
.* TLS +GLOBAL +DEFAULT +10 sg4
.* TLS +GLOBAL +DEFAULT +10 sg5
.* NOTYPE +GLOBAL +DEFAULT +UND __tls_get_addr
.* TLS +GLOBAL +DEFAULT +10 sg1
.* FUNC +GLOBAL +DEFAULT +7 fn1
.* TLS +GLOBAL +DEFAULT +10 sg2
.* TLS +GLOBAL +DEFAULT +10 sg6
.* TLS +GLOBAL +DEFAULT +10 sg7
