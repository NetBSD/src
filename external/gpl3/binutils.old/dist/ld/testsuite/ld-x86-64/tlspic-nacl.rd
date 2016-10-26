#source: tlspic1.s
#source: tlspic2.s
#as: --64
#ld: -shared -melf_x86_64_nacl --no-ld-generated-unwind-info
#readelf: -WSsrl
#target: x86_64-*-nacl*

There are [0-9]+ section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
 +\[Nr\] Name +Type +Address +Off +Size +ES Flg Lk Inf Al
 +\[[ 0-9]+\] +NULL +0+ 0+ 0+ 00 +0 +0 +0
 +\[[ 0-9]+\] .plt +.*
 +\[[ 0-9]+\] .text +PROGBITS +0+1000 [0-9a-f]+ 0+31a 00 +AX +0 +0 4096
 +\[[ 0-9]+\] .hash +.*
 +\[[ 0-9]+\] .dynsym +.*
 +\[[ 0-9]+\] .dynstr +.*
 +\[[ 0-9]+\] .rela.dyn +.*
 +\[[ 0-9]+\] .rela.plt +.*
 +\[[ 0-9]+\] .tdata +PROGBITS +0+100104e8 [0-9a-f]+ 0+60 00 WAT +0 +0 +1
 +\[[ 0-9]+\] .tbss +NOBITS +0+10010548 [0-9a-f]+ 0+20 00 WAT +0 +0 +1
 +\[[ 0-9]+\] .dynamic +DYNAMIC +0+10010548 [0-9a-f]+ 0+130 10 +WA +5 +0 +8
 +\[[ 0-9]+\] .got +PROGBITS +0+10010678 [0-9a-f]+ 0+90 08 +WA +0 +0 +8
 +\[[ 0-9]+\] .got.plt +PROGBITS +0+10010708 [0-9a-f]+ 0+20 08 +WA +0 +0 +8
 +\[[ 0-9]+\] .shstrtab +.*
 +\[[ 0-9]+\] .symtab +.*
 +\[[ 0-9]+\] .strtab +.*
Key to Flags:
.*
.*
.*

Elf file type is DYN \(Shared object file\)
Entry point 0x1000
There are [0-9]+ program headers, starting at offset [0-9]+

Program Headers:
 +Type +Offset +VirtAddr +PhysAddr +FileSiz +MemSiz +Flg Align
 +LOAD +0x0+10000 0x0+ 0x0+ 0x[0-9a-f]+ 0x[0-9a-f]+ R E 0x10000
 +LOAD +0x0+ 0x0+10000000 0x0+10000000 0x0+4e8 0x0+4e8 R +0x10000
 +LOAD +0x0+4e8 0x0+100104e8 0x0+100104e8 0x0+240 0x0+240 RW +0x10000
 +DYNAMIC +0x0+548 0x0+10010548 0x0+10010548 0x0+130 0x0+130 RW +0x8
 +TLS +0x0+4e8 0x0+100104e8 0x0+100104e8 0x0+60 0x0+80 R +0x1

 Section to Segment mapping:
 +Segment Sections...
 +00 +.plt .text *
 +01 +.hash .dynsym .dynstr .rela.dyn .rela.plt *
 +02 +.tdata .dynamic .got .got.plt *
 +03 +.dynamic *
 +04 +.tdata .tbss *

Relocation section '.rela.dyn' at offset 0x[0-9a-f]+ contains 14 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
[0-9a-f ]+R_X86_64_DTPMOD64 +0
[0-9a-f ]+R_X86_64_TPOFF64 +24
[0-9a-f ]+R_X86_64_TPOFF64 +30
[0-9a-f ]+R_X86_64_DTPMOD64 +0
[0-9a-f ]+R_X86_64_DTPMOD64 +0
[0-9a-f ]+R_X86_64_TPOFF64 +64
[0-9a-f ]+R_X86_64_TPOFF64 +50
[0-9a-f ]+R_X86_64_TPOFF64 +70
[0-9a-f ]+R_X86_64_DTPMOD64 +0
[0-9a-f ]+R_X86_64_TPOFF64 +44
[0-9a-f ]+R_X86_64_TPOFF64 +0+10 sg5 \+ 0
[0-9a-f ]+R_X86_64_DTPMOD64 +0+ sg1 \+ 0
[0-9a-f ]+R_X86_64_DTPOFF64 +0+ sg1 \+ 0
[0-9a-f ]+R_X86_64_TPOFF64 +0+4 sg2 \+ 0

Relocation section '.rela.plt' at offset 0x[0-9a-f]+ contains 1 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
[0-9a-f ]+R_X86_64_JUMP_SLOT +0+ __tls_get_addr \+ 0

Symbol table '\.dynsym' contains [0-9]+ entries:
 +Num: +Value +Size +Type +Bind +Vis +Ndx +Name
.* NOTYPE +LOCAL +DEFAULT +UND *
.* SECTION +LOCAL +DEFAULT +2 *
.* SECTION +LOCAL +DEFAULT +8 *
.* TLS +GLOBAL +DEFAULT +8 sg8
.* TLS +GLOBAL +DEFAULT +8 sg3
.* TLS +GLOBAL +DEFAULT +8 sg4
.* TLS +GLOBAL +DEFAULT +8 sg5
.* NOTYPE +GLOBAL +DEFAULT +UND __tls_get_addr
.* TLS +GLOBAL +DEFAULT +8 sg1
.* FUNC +GLOBAL +DEFAULT +2 fn1
.* NOTYPE +GLOBAL +DEFAULT +12 __bss_start
.* TLS +GLOBAL +DEFAULT +8 sg2
.* TLS +GLOBAL +DEFAULT +8 sg6
.* TLS +GLOBAL +DEFAULT +8 sg7
.* NOTYPE +GLOBAL +DEFAULT +12 _edata
.* NOTYPE +GLOBAL +DEFAULT +12 _end

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
.* FILE +LOCAL +DEFAULT +ABS tmpdir/tlspic1.o
.* TLS +LOCAL +DEFAULT +8 sl1
.* TLS +LOCAL +DEFAULT +8 sl2
.* TLS +LOCAL +DEFAULT +8 sl3
.* TLS +LOCAL +DEFAULT +8 sl4
.* TLS +LOCAL +DEFAULT +8 sl5
.* TLS +LOCAL +DEFAULT +8 sl6
.* TLS +LOCAL +DEFAULT +8 sl7
.* TLS +LOCAL +DEFAULT +8 sl8
.* FILE +LOCAL +DEFAULT +ABS 
.* TLS +LOCAL +DEFAULT +9 sH1
.* OBJECT +LOCAL +DEFAULT +10 _DYNAMIC
.* TLS +LOCAL +DEFAULT +8 sh3
.* TLS +LOCAL +DEFAULT +9 sH2
.* TLS +LOCAL +DEFAULT +9 sH7
.* TLS +LOCAL +DEFAULT +8 sh7
.* TLS +LOCAL +DEFAULT +8 sh8
.* TLS +LOCAL +DEFAULT +9 sH4
.* TLS +LOCAL +DEFAULT +8 sh4
.* TLS +LOCAL +DEFAULT +9 sH3
.* TLS +LOCAL +DEFAULT +8 sh5
.* TLS +LOCAL +DEFAULT +9 sH5
.* TLS +LOCAL +DEFAULT +9 sH6
.* TLS +LOCAL +DEFAULT +9 sH8
.* TLS +LOCAL +DEFAULT +8 sh1
.* OBJECT +LOCAL +DEFAULT +12 _GLOBAL_OFFSET_TABLE_
.* TLS +LOCAL +DEFAULT +8 sh2
.* TLS +LOCAL +DEFAULT +8 sh6
.* TLS +GLOBAL +DEFAULT +8 sg8
.* TLS +GLOBAL +DEFAULT +8 sg3
.* TLS +GLOBAL +DEFAULT +8 sg4
.* TLS +GLOBAL +DEFAULT +8 sg5
.* NOTYPE +GLOBAL +DEFAULT +UND __tls_get_addr
.* TLS +GLOBAL +DEFAULT +8 sg1
.* FUNC +GLOBAL +DEFAULT +2 fn1
.* NOTYPE +GLOBAL +DEFAULT +12 __bss_start
.* TLS +GLOBAL +DEFAULT +8 sg2
.* TLS +GLOBAL +DEFAULT +8 sg6
.* TLS +GLOBAL +DEFAULT +8 sg7
.* NOTYPE +GLOBAL +DEFAULT +12 _edata
.* NOTYPE +GLOBAL +DEFAULT +12 _end
