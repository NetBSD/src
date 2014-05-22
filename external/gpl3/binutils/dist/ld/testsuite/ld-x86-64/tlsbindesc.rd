#source: tlsbindesc.s
#source: tlsbin.s
#as: --64
#ld: -shared -melf_x86_64 --no-ld-generated-unwind-info
#readelf: -WSsrl
#target: x86_64-*-*

There are [0-9]+ section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
 +\[Nr\] Name +Type +Address +Off +Size +ES Flg Lk Inf Al
 +\[[ 0-9]+\] +NULL +0+ 0+ 0+ 00 +0 +0 +0
 +\[[ 0-9]+\] .interp +.*
 +\[[ 0-9]+\] .hash +.*
 +\[[ 0-9]+\] .dynsym +.*
 +\[[ 0-9]+\] .dynstr +.*
 +\[[ 0-9]+\] .rela.dyn +.*
 +\[[ 0-9]+\] .text +PROGBITS +0+401000 0+1000 0+200 00 +AX +0 +0 +4096
 +\[[ 0-9]+\] .tdata +PROGBITS +0+601200 0+1200 0+60 00 WAT +0 +0 +1
 +\[[ 0-9]+\] .tbss +NOBITS +0+601260 0+1260 0+40 00 WAT +0 +0 +1
 +\[[ 0-9]+\] .dynamic +DYNAMIC +0+601260 0+1260 0+100 10 +WA +4 +0 +8
 +\[[ 0-9]+\] .got +PROGBITS +0+601360 0+1360 0+20 08 +WA +0 +0 +8
 +\[[ 0-9]+\] .got.plt +PROGBITS +0+601380 0+1380 0+18 08 +WA +0 +0 +8
 +\[[ 0-9]+\] .shstrtab +.*
 +\[[ 0-9]+\] .symtab +.*
 +\[[ 0-9]+\] .strtab +.*
Key to Flags:
.*
.*
.*

Elf file type is EXEC \(Executable file\)
Entry point 0x401108
There are [0-9]+ program headers, starting at offset [0-9]+

Program Headers:
 +Type +Offset +VirtAddr +PhysAddr +FileSiz +MemSiz +Flg Align
 +PHDR.*
 +INTERP.*
.*Requesting program interpreter.*
 +LOAD +0x0+ 0x0+400000 0x0+400000 0x0+1200 0x0+1200 R E 0x200000
 +LOAD +0x0+1200 0x0+601200 0x0+601200 0x0+198 0x0+198 RW +0x200000
 +DYNAMIC +0x0+1260 0x0+601260 0x0+601260 0x0+100 0x0+100 RW +0x8
 +TLS +0x0+1200 0x0+601200 0x0+601200 0x0+60 0x0+a0 R +0x1

 Section to Segment mapping:
 +Segment Sections...
 +00 *
 +01 +.interp *
 +02 +.interp .hash .dynsym .dynstr .rela.dyn .text *
 +03 +.tdata .dynamic .got .got.plt *
 +04 +.dynamic *
 +05 +.tdata .tbss *

Relocation section '.rela.dyn' at offset 0x[0-9a-f]+ contains 4 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+601360 +0+100000012 R_X86_64_TPOFF64 +0+ sG5 \+ 0
0+601368 +0+200000012 R_X86_64_TPOFF64 +0+ sG2 \+ 0
0+601370 +0+400000012 R_X86_64_TPOFF64 +0+ sG6 \+ 0
0+601378 +0+500000012 R_X86_64_TPOFF64 +0+ sG1 \+ 0

Symbol table '\.dynsym' contains [0-9]+ entries:
 +Num: +Value +Size +Type +Bind +Vis +Ndx +Name
 +[0-9]+: 0+ +0 +NOTYPE +LOCAL +DEFAULT +UND *
 +[0-9]+: 0+ +0 +TLS +GLOBAL +DEFAULT +UND sG5
 +[0-9]+: 0+ +0 +TLS +GLOBAL +DEFAULT +UND sG2
 +[0-9]+: 0+[0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +11 __bss_start
 +[0-9]+: 0+ +0 +TLS +GLOBAL +DEFAULT +UND sG6
 +[0-9]+: 0+ +0 +TLS +GLOBAL +DEFAULT +UND sG1
 +[0-9]+: 0+[0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +11 _edata
 +[0-9]+: 0+[0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +11 _end

Symbol table '\.symtab' contains [0-9]+ entries:
 +Num: +Value +Size +Type +Bind +Vis +Ndx +Name
 +[0-9]+: 0+ +0 +NOTYPE +LOCAL +DEFAULT +UND *
 +[0-9]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +1 *
 +[0-9]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +2 *
 +[0-9]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +3 *
 +[0-9]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +4 *
 +[0-9]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +5 *
 +[0-9]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +6 *
 +[0-9]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +7 *
 +[0-9]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +8 *
 +[0-9]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +9 *
 +[0-9]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +10 *
 +[0-9]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +11 *
.* FILE +LOCAL +DEFAULT +ABS .*
 +[0-9]+: 0+20 +0 +TLS +LOCAL +DEFAULT +7 sl1
 +[0-9]+: 0+24 +0 +TLS +LOCAL +DEFAULT +7 sl2
 +[0-9]+: 0+28 +0 +TLS +LOCAL +DEFAULT +7 sl3
 +[0-9]+: 0+2c +0 +TLS +LOCAL +DEFAULT +7 sl4
 +[0-9]+: 0+30 +0 +TLS +LOCAL +DEFAULT +7 sl5
 +[0-9]+: 0+34 +0 +TLS +LOCAL +DEFAULT +7 sl6
 +[0-9]+: 0+38 +0 +TLS +LOCAL +DEFAULT +7 sl7
 +[0-9]+: 0+3c +0 +TLS +LOCAL +DEFAULT +7 sl8
.* FILE +LOCAL +DEFAULT +ABS .*
 +[0-9]+: 0+80 +0 +TLS +LOCAL +DEFAULT +8 bl1
 +[0-9]+: 0+84 +0 +TLS +LOCAL +DEFAULT +8 bl2
 +[0-9]+: 0+88 +0 +TLS +LOCAL +DEFAULT +8 bl3
 +[0-9]+: 0+8c +0 +TLS +LOCAL +DEFAULT +8 bl4
 +[0-9]+: 0+90 +0 +TLS +LOCAL +DEFAULT +8 bl5
 +[0-9]+: 0+94 +0 +TLS +LOCAL +DEFAULT +8 bl6
 +[0-9]+: 0+98 +0 +TLS +LOCAL +DEFAULT +8 bl7
 +[0-9]+: 0+9c +0 +TLS +LOCAL +DEFAULT +8 bl8
.* FILE +LOCAL +DEFAULT +ABS .*
 +[0-9]+: 0+a0 +0 +TLS +LOCAL +DEFAULT +7 _TLS_MODULE_BASE_
 +[0-9]+: 0+601260 +0 +OBJECT +LOCAL +DEFAULT +9 _DYNAMIC
 +[0-9]+: 0+601380 +0 +OBJECT +LOCAL +DEFAULT +11 _GLOBAL_OFFSET_TABLE_
 +[0-9]+: 0+1c +0 +TLS +GLOBAL +DEFAULT +7 sg8
 +[0-9]+: 0+7c +0 +TLS +GLOBAL +DEFAULT +8 bg8
 +[0-9]+: 0+74 +0 +TLS +GLOBAL +DEFAULT +8 bg6
 +[0-9]+: 0+ +0 +TLS +GLOBAL +DEFAULT +UND sG5
 +[0-9]+: 0+68 +0 +TLS +GLOBAL +DEFAULT +8 bg3
 +[0-9]+: 0+8 +0 +TLS +GLOBAL +DEFAULT +7 sg3
 +[0-9]+: 0+48 +0 +TLS +GLOBAL +HIDDEN +7 sh3
 +[0-9]+: 0+ +0 +TLS +GLOBAL +DEFAULT +UND sG2
 +[0-9]+: 0+c +0 +TLS +GLOBAL +DEFAULT +7 sg4
 +[0-9]+: 0+10 +0 +TLS +GLOBAL +DEFAULT +7 sg5
 +[0-9]+: 0+70 +0 +TLS +GLOBAL +DEFAULT +8 bg5
 +[0-9]+: 0+58 +0 +TLS +GLOBAL +HIDDEN +7 sh7
 +[0-9]+: 0+5c +0 +TLS +GLOBAL +HIDDEN +7 sh8
 +[0-9]+: 0+ +0 +TLS +GLOBAL +DEFAULT +7 sg1
 +[0-9]+: 0+401108 +0 +FUNC +GLOBAL +DEFAULT +6 _start
 +[0-9]+: 0+4c +0 +TLS +GLOBAL +HIDDEN +7 sh4
 +[0-9]+: 0+78 +0 +TLS +GLOBAL +DEFAULT +8 bg7
 +[0-9]+: 0+50 +0 +TLS +GLOBAL +HIDDEN +7 sh5
 +[0-9]+: [0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +11 __bss_start
 +[0-9]+: 0+ +0 +TLS +GLOBAL +DEFAULT +UND sG6
 +[0-9]+: 0+401000 +0 +FUNC +GLOBAL +DEFAULT +6 fn2
 +[0-9]+: 0+4 +0 +TLS +GLOBAL +DEFAULT +7 sg2
 +[0-9]+: 0+ +0 +TLS +GLOBAL +DEFAULT +UND sG1
 +[0-9]+: 0+40 +0 +TLS +GLOBAL +HIDDEN +7 sh1
 +[0-9]+: 0+14 +0 +TLS +GLOBAL +DEFAULT +7 sg6
 +[0-9]+: 0+18 +0 +TLS +GLOBAL +DEFAULT +7 sg7
 +[0-9]+: [0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +11 _edata
 +[0-9]+: [0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +11 _end
 +[0-9]+: 0+44 +0 +TLS +GLOBAL +HIDDEN +7 sh2
 +[0-9]+: 0+54 +0 +TLS +GLOBAL +HIDDEN +7 sh6
 +[0-9]+: 0+64 +0 +TLS +GLOBAL +DEFAULT +8 bg2
 +[0-9]+: 0+60 +0 +TLS +GLOBAL +DEFAULT +8 bg1
 +[0-9]+: 0+6c +0 +TLS +GLOBAL +DEFAULT +8 bg4
