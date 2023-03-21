#source: tlsbinpic.s
#source: tlsbin.s
#as:
#ld: -shared
#readelf: -WSsrl
#target: ia64-*-*

There are [0-9]+ section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
 +\[Nr\] Name +Type +Address +Off +Size +ES Flg Lk Inf Al
 +\[[ 0-9]+\] +NULL +0+ 0+ 0+ 00 +0 +0 +0
 +\[[ 0-9]+\] .interp +.*
 +\[[ 0-9]+\] .hash +.*
 +\[[ 0-9]+\] .dynsym +.*
 +\[[ 0-9]+\] .dynstr +.*
 +\[[ 0-9]+\] .rela.dyn +.*
 +\[[ 0-9]+\] .rela.IA_64.pltoff +.*
 +\[[ 0-9]+\] .plt +.*
 +\[[ 0-9]+\] .text +PROGBITS +40+1000 0+1000 0+140 00 +AX +0 +0 4096
 +\[[ 0-9]+\] .IA_64.unwind_info +.*
 +\[[ 0-9]+\] .IA_64.unwind +.*
 +\[[ 0-9]+\] .tdata +PROGBITS +60+1[0-9a-f]+ 0+1[0-9a-f]+ 0+60 00 WAT +0 +0 +4
 +\[[ 0-9]+\] .tbss +NOBITS +60+1[0-9a-f]+ 0+1[0-9a-f]+ 0+40 00 WAT +0 +0 +1
 +\[[ 0-9]+\] .dynamic +DYNAMIC +60+1[0-9a-f]+ 0+1[0-9a-f]+ 0+150 10 +WA +4 +0 +8
 +\[[ 0-9]+\] .got +PROGBITS +60+1318 0+1318 0+48 00 WAp +0 +0 +8
 +\[[ 0-9]+\] .IA_64.pltoff +.*
 +\[[ 0-9]+\] .symtab +.*
 +\[[ 0-9]+\] .strtab +.*
 +\[[ 0-9]+\] .shstrtab +.*
#...

Elf file type is EXEC \(Executable file\)
Entry point 0x40+10d0
There are [0-9]+ program headers, starting at offset [0-9]+

Program Headers:
 +Type +Offset +VirtAddr +PhysAddr +FileSiz +MemSiz +Flg Align
 +PHDR +0x0+40 0x40+40 0x40+40 0x0+188 0x0+188 R +0x8
 +INTERP +0x0+1c8 0x40+1c8 0x40+1c8 0x[0-9a-f]+ 0x[0-9a-f]+ R +0x1
.*Requesting program interpreter.*
 +LOAD +0x0+ 0x40+ 0x40+ 0x0+1[0-9a-f]+ 0x0+1[0-9a-f]+ R E 0x10000
 +LOAD +0x0+1[0-9a-f]+ 0x60+1[0-9a-f]+ 0x60+1[0-9a-f]+ 0x0+0[0-9a-f]+ 0x0+0[0-9a-f]+ RW +0x10000
 +DYNAMIC +0x0+1[0-9a-f]+ 0x60+1[0-9a-f]+ 0x60+1[0-9a-f]+ 0x0+150 0x0+150 RW +0x8
 +TLS +0x0+1[0-9a-f]+ 0x60+1[0-9a-f]+ 0x60+1[0-9a-f]+ 0x0+60 0x0+a0 R +0x4
 +IA_64_UNWIND .* R +0x8
#...

Relocation section '.rela.dyn' at offset 0x[0-9a-f]+ contains 3 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
[0-9a-f ]+R_IA64_TPREL64LSB +0+ sG2 \+ 0
[0-9a-f ]+R_IA64_DTPMOD64LSB +0+ sG1 \+ 0
[0-9a-f ]+R_IA64_DTPREL64LSB +0+ sG1 \+ 0

Relocation section '.rela.IA_64.pltoff' at offset 0x[0-9a-f]+ contains 1 entry:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
[0-9a-f ]+R_IA64_IPLTLSB +0+ __tls_get_addr \+ 0

Symbol table '\.dynsym' contains [0-9]+ entries:
 +Num: +Value +Size +Type +Bind +Vis +Ndx +Name
.* NOTYPE +LOCAL +DEFAULT +UND *
.* TLS +GLOBAL +DEFAULT +UND sG2
.* FUNC +GLOBAL +DEFAULT +UND __tls_get_addr
.* TLS +GLOBAL +DEFAULT +UND sG1

Symbol table '\.symtab' contains [0-9]+ entries:
 +Num: +Value +Size +Type +Bind +Vis +Ndx +Name
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
.* SECTION +LOCAL +DEFAULT +15 *
.* FILE +LOCAL +DEFAULT +ABS .*
.* TLS +LOCAL +DEFAULT +11 sl1
.* TLS +LOCAL +DEFAULT +11 sl2
.* TLS +LOCAL +DEFAULT +11 sl3
.* TLS +LOCAL +DEFAULT +11 sl4
.* TLS +LOCAL +DEFAULT +11 sl5
.* TLS +LOCAL +DEFAULT +11 sl6
.* TLS +LOCAL +DEFAULT +11 sl7
.* TLS +LOCAL +DEFAULT +11 sl8
.* FILE +LOCAL +DEFAULT +ABS .*
.* TLS +LOCAL +DEFAULT +12 bl1
.* TLS +LOCAL +DEFAULT +12 bl2
.* TLS +LOCAL +DEFAULT +12 bl3
.* TLS +LOCAL +DEFAULT +12 bl4
.* TLS +LOCAL +DEFAULT +12 bl5
.* TLS +LOCAL +DEFAULT +12 bl6
.* TLS +LOCAL +DEFAULT +12 bl7
.* TLS +LOCAL +DEFAULT +12 bl8
.* FILE +LOCAL +DEFAULT +ABS .*
.* OBJECT +LOCAL +DEFAULT +13 _DYNAMIC
.* OBJECT +LOCAL +DEFAULT +14 _GLOBAL_OFFSET_TABLE_
.* TLS +GLOBAL +DEFAULT +11 sg8
.* TLS +GLOBAL +DEFAULT +12 bg8
.* TLS +GLOBAL +DEFAULT +12 bg6
.* TLS +GLOBAL +DEFAULT +12 bg3
.* TLS +GLOBAL +DEFAULT +11 sg3
.* TLS +GLOBAL +HIDDEN +11 sh3
.* TLS +GLOBAL +DEFAULT +UND sG2
.* TLS +GLOBAL +DEFAULT +11 sg4
.* TLS +GLOBAL +DEFAULT +11 sg5
.* TLS +GLOBAL +DEFAULT +12 bg5
.* FUNC +GLOBAL +DEFAULT +UND __tls_get_addr
.* TLS +GLOBAL +HIDDEN +11 sh7
.* TLS +GLOBAL +HIDDEN +11 sh8
.* TLS +GLOBAL +DEFAULT +11 sg1
.* FUNC +GLOBAL +DEFAULT +8 _start
.* TLS +GLOBAL +HIDDEN +11 sh4
.* TLS +GLOBAL +DEFAULT +12 bg7
.* TLS +GLOBAL +HIDDEN +11 sh5
.* NOTYPE +GLOBAL +DEFAULT +15 __bss_start
.* FUNC +GLOBAL +DEFAULT +8 fn2
.* TLS +GLOBAL +DEFAULT +11 sg2
.* TLS +GLOBAL +DEFAULT +UND sG1
.* TLS +GLOBAL +HIDDEN +11 sh1
.* TLS +GLOBAL +DEFAULT +11 sg6
.* TLS +GLOBAL +DEFAULT +11 sg7
.* NOTYPE +GLOBAL +DEFAULT +15 _edata
.* NOTYPE +GLOBAL +DEFAULT +15 _end
.* TLS +GLOBAL +HIDDEN +11 sh2
.* TLS +GLOBAL +HIDDEN +11 sh6
.* TLS +GLOBAL +DEFAULT +12 bg2
.* TLS +GLOBAL +DEFAULT +12 bg1
.* TLS +GLOBAL +DEFAULT +12 bg4
