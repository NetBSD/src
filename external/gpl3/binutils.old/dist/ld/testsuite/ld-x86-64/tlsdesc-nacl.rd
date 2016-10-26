#source: tlsdesc.s
#source: tlspic2.s
#as: --64
#ld: -shared -melf_x86_64_nacl --no-ld-generated-unwind-info
#readelf: -WSsrld
#target: x86_64-*-nacl*

There are [0-9]+ section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
 +\[Nr\] Name +Type +Address +Off +Size +ES Flg Lk Inf Al
 +\[[ 0-9]+\] +NULL +0+ 0+ 0+ 00 +0 +0 +0
 +\[[ 0-9]+\] .plt +PROGBITS +0+ [0-9a-f]+ +0+80 +40 +AX +0 +0 +32
 +\[[ 0-9]+\] .text +PROGBITS +0+1000 [0-9a-f]+ +0+153 00 +AX +0 +0 4096
 +\[[ 0-9]+\] .hash +.*
 +\[[ 0-9]+\] .dynsym +.*
 +\[[ 0-9]+\] .dynstr +.*
 +\[[ 0-9]+\] .rela.dyn +.*
 +\[[ 0-9]+\] .rela.plt +.*
 +\[[ 0-9]+\] .tdata +PROGBITS +0+10010488 [0-9a-f]+ +0+60 00 WAT +0 +0 +1
 +\[[ 0-9]+\] .tbss +NOBITS +0+100104e8 [0-9a-f]+ 0+20 00 WAT +0 +0 +1
 +\[[ 0-9]+\] .dynamic +DYNAMIC +0+100104e8 [0-9a-f]+ 0+150 10 +WA +5 +0 +8
 +\[[ 0-9]+\] .got +PROGBITS +0+10010638 [0-9a-f]+ 0+48 08 +WA +0 +0 +8
 +\[[ 0-9]+\] .got.plt +PROGBITS +0+10010680 [0-9a-f]+ 0+68 08 +WA +0 +0 +8
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
 +LOAD +0x[0-9a-f]+ 0x0+ 0x0+ 0x[0-9a-f]+ 0x[0-9a-f]+ R E 0x10000
 +LOAD +0x[0-9a-f]+ 0x0+10000000 0x0+10000000 0x[0-9a-f]+ 0x[0-9a-f]+ R +0x10000
 +LOAD +0x[0-9a-f]+ 0x0+10010488 0x0+10010488 0x0+260 0x0+260 RW +0x10000
 +DYNAMIC +0x[0-9a-f]+ 0x0+100104e8 0x0+100104e8 0x0+150 0x0+150 RW +0x8
 +TLS +0x[0-9a-f]+ 0x0+10010488 0x0+10010488 0x0+60 0x0+80 R +0x1

 Section to Segment mapping:
 +Segment Sections...
 +00 +.plt .text *
 +01 +.hash .dynsym .dynstr .rela.dyn .rela.plt *
 +02 +.tdata .dynamic .got .got.plt *
 +03 +.dynamic *
 +04 +.tdata .tbss *

Dynamic section at offset 0x[0-9a-f]+ contains 16 entries:
 +Tag +Type +Name/Value
 0x[0-9a-f]+ +\(HASH\).*
 0x[0-9a-f]+ +\(STRTAB\).*
 0x[0-9a-f]+ +\(SYMTAB\).*
 0x[0-9a-f]+ +\(STRSZ\).*
 0x[0-9a-f]+ +\(SYMENT\).*
 0x[0-9a-f]+ +\(PLTGOT\).*
 0x[0-9a-f]+ +\(PLTRELSZ\).*
 0x[0-9a-f]+ +\(PLTREL\).*
 0x[0-9a-f]+ +\(JMPREL\).*
 0x[0-9a-f]+ +\(TLSDESC_PLT\) +0x40
 0x[0-9a-f]+ +\(TLSDESC_GOT\) +0x10010678
 0x[0-9a-f]+ +\(RELA\).*
 0x[0-9a-f]+ +\(RELASZ\).*
 0x[0-9a-f]+ +\(RELAENT\).*
 0x[0-9a-f]+ +\(FLAGS\).*
 0x[0-9a-f]+ +\(NULL\).*

Relocation section '.rela.dyn' at offset 0x[0-9a-f]+ contains 8 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+10010638 +[0-9a-f]+ R_X86_64_TPOFF64 +24
0+10010640 +[0-9a-f]+ R_X86_64_TPOFF64 +30
0+10010648 +[0-9a-f]+ R_X86_64_TPOFF64 +64
0+10010658 +[0-9a-f]+ R_X86_64_TPOFF64 +50
0+10010660 +[0-9a-f]+ R_X86_64_TPOFF64 +70
0+10010670 +[0-9a-f]+ R_X86_64_TPOFF64 +44
0+10010650 +[0-9a-f]+ R_X86_64_TPOFF64 +0+10 sg5 \+ 0
0+10010668 +[0-9a-f]+ R_X86_64_TPOFF64 +0+4 sg2 \+ 0

Relocation section '.rela.plt' at offset 0x[0-9a-f]+ contains 5 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+100106c8 +[0-9a-f]+ R_X86_64_TLSDESC +0+ sg1 \+ 0
0+10010698 +[0-9a-f]+ R_X86_64_TLSDESC +20
0+100106d8 +[0-9a-f]+ R_X86_64_TLSDESC +40
0+100106a8 +[0-9a-f]+ R_X86_64_TLSDESC +60
0+100106b8 +[0-9a-f]+ R_X86_64_TLSDESC +0

Symbol table '\.dynsym' contains [0-9]+ entries:
 +Num: +Value +Size +Type +Bind +Vis +Ndx +Name
 +[0-9]+: 0+ +0 +NOTYPE +LOCAL +DEFAULT +UND *
 +[0-9]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +2 *
 +[0-9]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +8 *
 +[0-9]+: 0+1c +0 +TLS +GLOBAL +DEFAULT +8 sg8
 +[0-9]+: 0+8 +0 +TLS +GLOBAL +DEFAULT +8 sg3
 +[0-9]+: 0+c +0 +TLS +GLOBAL +DEFAULT +8 sg4
 +[0-9]+: 0+10 +0 +TLS +GLOBAL +DEFAULT +8 sg5
 +[0-9]+: 0+ +0 +TLS +GLOBAL +DEFAULT +8 sg1
 +[0-9]+: 0+1000 +0 +FUNC +GLOBAL +DEFAULT +2 fn1
 +[0-9]+: [0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +12 __bss_start
 +[0-9]+: 0+4 +0 +TLS +GLOBAL +DEFAULT +8 sg2
 +[0-9]+: 0+14 +0 +TLS +GLOBAL +DEFAULT +8 sg6
 +[0-9]+: 0+18 +0 +TLS +GLOBAL +DEFAULT +8 sg7
 +[0-9]+: [0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +12 _edata
 +[0-9]+: [0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +12 _end

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
 +[0-9]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +12 *
.* FILE +LOCAL +DEFAULT +ABS tmpdir/tlsdesc.o
 +[0-9]+: 0+20 +0 +TLS +LOCAL +DEFAULT +8 sl1
 +[0-9]+: 0+24 +0 +TLS +LOCAL +DEFAULT +8 sl2
 +[0-9]+: 0+28 +0 +TLS +LOCAL +DEFAULT +8 sl3
 +[0-9]+: 0+2c +0 +TLS +LOCAL +DEFAULT +8 sl4
 +[0-9]+: 0+30 +0 +TLS +LOCAL +DEFAULT +8 sl5
 +[0-9]+: 0+34 +0 +TLS +LOCAL +DEFAULT +8 sl6
 +[0-9]+: 0+38 +0 +TLS +LOCAL +DEFAULT +8 sl7
 +[0-9]+: 0+3c +0 +TLS +LOCAL +DEFAULT +8 sl8
.* FILE +LOCAL +DEFAULT +ABS 
 +[0-9]+: 0+60 +0 +TLS +LOCAL +DEFAULT +9 sH1
 +[0-9]+: 0+ +0 +TLS +LOCAL +DEFAULT +8 _TLS_MODULE_BASE_
 +[0-9]+: 0+100104e8 +0 +OBJECT +LOCAL +DEFAULT +10 _DYNAMIC
 +[0-9]+: 0+48 +0 +TLS +LOCAL +DEFAULT +8 sh3
 +[0-9]+: 0+64 +0 +TLS +LOCAL +DEFAULT +9 sH2
 +[0-9]+: 0+78 +0 +TLS +LOCAL +DEFAULT +9 sH7
 +[0-9]+: 0+58 +0 +TLS +LOCAL +DEFAULT +8 sh7
 +[0-9]+: 0+5c +0 +TLS +LOCAL +DEFAULT +8 sh8
 +[0-9]+: 0+6c +0 +TLS +LOCAL +DEFAULT +9 sH4
 +[0-9]+: 0+4c +0 +TLS +LOCAL +DEFAULT +8 sh4
 +[0-9]+: 0+68 +0 +TLS +LOCAL +DEFAULT +9 sH3
 +[0-9]+: 0+50 +0 +TLS +LOCAL +DEFAULT +8 sh5
 +[0-9]+: 0+70 +0 +TLS +LOCAL +DEFAULT +9 sH5
 +[0-9]+: 0+74 +0 +TLS +LOCAL +DEFAULT +9 sH6
 +[0-9]+: 0+7c +0 +TLS +LOCAL +DEFAULT +9 sH8
 +[0-9]+: 0+40 +0 +TLS +LOCAL +DEFAULT +8 sh1
 +[0-9]+: 0+10010680 +0 +OBJECT +LOCAL +DEFAULT +12 _GLOBAL_OFFSET_TABLE_
 +[0-9]+: 0+44 +0 +TLS +LOCAL +DEFAULT +8 sh2
 +[0-9]+: 0+54 +0 +TLS +LOCAL +DEFAULT +8 sh6
 +[0-9]+: 0+1c +0 +TLS +GLOBAL +DEFAULT +8 sg8
 +[0-9]+: 0+8 +0 +TLS +GLOBAL +DEFAULT +8 sg3
 +[0-9]+: 0+c +0 +TLS +GLOBAL +DEFAULT +8 sg4
 +[0-9]+: 0+10 +0 +TLS +GLOBAL +DEFAULT +8 sg5
 +[0-9]+: 0+ +0 +TLS +GLOBAL +DEFAULT +8 sg1
 +[0-9]+: 0+1000 +0 +FUNC +GLOBAL +DEFAULT +2 fn1
 +[0-9]+: [0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +12 __bss_start
 +[0-9]+: 0+4 +0 +TLS +GLOBAL +DEFAULT +8 sg2
 +[0-9]+: 0+14 +0 +TLS +GLOBAL +DEFAULT +8 sg6
 +[0-9]+: 0+18 +0 +TLS +GLOBAL +DEFAULT +8 sg7
 +[0-9]+: [0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +12 _edata
 +[0-9]+: [0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +12 _end
