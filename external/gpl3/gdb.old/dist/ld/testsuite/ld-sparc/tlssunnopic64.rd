#source: tlssunnopic64.s
#source: tlsnopic.s
#as: --64 -Av9
#ld: -shared -melf64_sparc
#readelf: -WSsrl
#target: sparc*-*-*

.*

Section Headers:
 +\[Nr\] Name +Type +Address +Off +Size +ES Flg Lk Inf Al
 +\[[ 0-9]+\] +NULL +0+ 0+ 0+ 0+ +0 +0 +0
 +\[[ 0-9]+\] .hash +.*
 +\[[ 0-9]+\] .dynsym +.*
 +\[[ 0-9]+\] .dynstr +.*
 +\[[ 0-9]+\] .rela.dyn +.*
 +\[[ 0-9]+\] .text +PROGBITS +0+1000 0+1000 0+1000 0+ +AX +0 +0 4096
 +\[[ 0-9]+\] .tbss +NOBITS +0+102000 0+2000 0+24 0+ WAT +0 +0 +4
 +\[[ 0-9]+\] .dynamic +DYNAMIC +0+102000 0+2000 0+100 10 +WA +3 +0 +8
 +\[[ 0-9]+\] .got +PROGBITS +0+102100 0+2100 0+38 08 +WA +0 +0 +8
 +\[[ 0-9]+\] .symtab +.*
 +\[[ 0-9]+\] .strtab +.*
 +\[[ 0-9]+\] .shstrtab +.*
#...
Elf file type is DYN \(Shared object file\)
Entry point 0x1000
There are [0-9]+ program headers, starting at offset [0-9]+

Program Headers:
 +Type +Offset +VirtAddr +PhysAddr +FileSiz +MemSiz +Flg Align
 +LOAD .* R E 0x100000
 +LOAD .* RW +0x100000
 +DYNAMIC .* RW +0x8
 +TLS .* 0x0+ 0x0+24 R +0x4
#...

Relocation section '.rela.dyn' at offset 0x[0-9a-f]+ contains 14 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
[0-9a-f ]+R_SPARC_HH22 +0+102100 +\.got \+ 102100
[0-9a-f ]+R_SPARC_LM22 +0+102100 +\.got \+ 102100
[0-9a-f ]+R_SPARC_HM10 +0+102100 +\.got \+ 102100
[0-9a-f ]+R_SPARC_LO10 +0+102100 +\.got \+ 102100
[0-9a-f ]+R_SPARC_TLS_LE_HIX22 +9
[0-9a-f ]+R_SPARC_TLS_LE_LOX10 +9
[0-9a-f ]+R_SPARC_TLS_LE_HIX22 +1c
[0-9a-f ]+R_SPARC_TLS_LE_LOX10 +1c
[0-9a-f ]+R_SPARC_TLS_TPOFF64 +0
[0-9a-f ]+R_SPARC_TLS_TPOFF64 +4
[0-9a-f ]+R_SPARC_TLS_TPOFF64 +14
[0-9a-f ]+R_SPARC_TLS_TPOFF64 +18
[0-9a-f ]+R_SPARC_TLS_TPOFF64 +0+ +sg1 \+ 0
[0-9a-f ]+R_SPARC_TLS_TPOFF64 +0+ +sg2 \+ 0

Symbol table '\.dynsym' contains [0-9]+ entries:
 +Num: +Value +Size +Type +Bind +Vis +Ndx +Name
.* NOTYPE +LOCAL +DEFAULT +UND *
.* SECTION +LOCAL +DEFAULT +5 *
.* SECTION +LOCAL +DEFAULT +8 *
.* FUNC +GLOBAL +DEFAULT +5 fn3
.* TLS +GLOBAL +DEFAULT +UND sg1
.* TLS +GLOBAL +DEFAULT +UND sg2

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
.* FILE +LOCAL +DEFAULT +ABS .*
.* TLS +LOCAL +DEFAULT +6 bl1
.* TLS +LOCAL +DEFAULT +6 bl2
.* TLS +LOCAL +DEFAULT +6 bl3
.* TLS +LOCAL +DEFAULT +6 bl4
.* TLS +LOCAL +DEFAULT +6 bl5
.* FILE +LOCAL +DEFAULT +ABS 
.* OBJECT +LOCAL +DEFAULT +ABS _DYNAMIC
.* TLS +LOCAL +DEFAULT +6 sh3
.* OBJECT +LOCAL +DEFAULT +ABS _PROCEDURE_LINKAGE_TABLE_
.* TLS +LOCAL +DEFAULT +6 sh4
.* TLS +LOCAL +DEFAULT +6 sh1
.* OBJECT +LOCAL +DEFAULT +ABS _GLOBAL_OFFSET_TABLE_
.* TLS +LOCAL +DEFAULT +6 sh2
.* FUNC +GLOBAL +DEFAULT +5 fn3
.* TLS +GLOBAL +DEFAULT +UND sg1
.* TLS +GLOBAL +DEFAULT +UND sg2
