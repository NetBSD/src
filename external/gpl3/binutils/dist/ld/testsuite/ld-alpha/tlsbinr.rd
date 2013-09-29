#source: align.s
#source: tlsbinpic.s
#source: tlsbin.s
#as:
#ld: -relax -melf64alpha
#readelf: -WSsrl
#target: alpha*-*-*

There are [0-9]+ section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
 +\[Nr\] Name +Type +Address +Off +Size +ES Flg Lk Inf Al
 +\[[ 0-9]+\] +NULL +0+ 0+ 0+ 0+ +0 +0 +0
 +\[[ 0-9]+\] \.interp +.*
 +\[[ 0-9]+\] \.hash +.*
 +\[[ 0-9]+\] \.dynsym +.*
 +\[[ 0-9]+\] \.dynstr +.*
 +\[[ 0-9]+\] \.rela\.dyn +.*
 +\[[ 0-9]+\] \.rela\.plt +.*
 +\[[ 0-9]+\] \.text +PROGBITS +[0-9a-f]+ [0-9a-f]+ [0-9a-f]+ 0+ +AX +0 +0 4096
 +\[[ 0-9]+\] \.eh_frame +PROGBITS +[0-9a-f]+ [0-9a-f]+ [0-9a-f]+ 00 +A +0 +0 +8
 +\[[ 0-9]+\] \.tdata +PROGBITS +[0-9a-f]+ [0-9a-f]+ [0-9a-f]+ 0+ WAT +0 +0 +4
 +\[[ 0-9]+\] \.tbss +NOBITS +[0-9a-f]+ [0-9a-f]+ [0-9a-f]+ 0+ WAT +0 +0 +1
 +\[[ 0-9]+\] \.dynamic +DYNAMIC +[0-9a-f]+ [0-9a-f]+ [0-9a-f]+ 10 +WA +4 +0 +8
 +\[[ 0-9]+\] \.plt +PROGBITS +[0-9a-f]+ [0-9a-f]+ [0-9a-f]+ 0+ WAX +0 +0 +16
 +\[[ 0-9]+\] \.got +PROGBITS +[0-9a-f]+ [0-9a-f]+ [0-9a-f]+ 0+ +WA +0 +0 +8
 +\[[ 0-9]+\] \.shstrtab +.*
 +\[[ 0-9]+\] \.symtab +.*
 +\[[ 0-9]+\] \.strtab +.*
#...

Elf file type is EXEC \(Executable file\)
Entry point 0x[0-9a-f]+
There are [0-9]+ program headers, starting at offset [0-9]+

Program Headers:
 +Type +Offset +VirtAddr +PhysAddr +FileSiz +MemSiz +Flg Align
 +PHDR +0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ R E 0x8
 +INTERP +0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ R +0x1
.*Requesting program interpreter.*
 +LOAD +0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ R E 0x10000
 +LOAD +0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ RWE 0x10000
 +DYNAMIC +0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ RW +0x8
 +TLS +0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ 0x[0-9a-f]+ R +0x4
#...

Relocation section '.rela.dyn' at offset 0x[0-9a-f]+ contains 2 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
[0-9a-f]+ +0+100000026 R_ALPHA_TPREL64 +0+ sG2 \+ 0
[0-9a-f]+ +0+400000026 R_ALPHA_TPREL64 +0+ sG1 \+ 0

Symbol table '\.dynsym' contains [0-9]+ entries:
 +Num: +Value +Size +Type +Bind +Vis +Ndx +Name
[0-9 ]+: 0+ +0 +NOTYPE +LOCAL +DEFAULT +UND 
[0-9 ]+: 0+ +0 +TLS +GLOBAL +DEFAULT +UND sG2
[0-9 ]+: 0+ +0 +FUNC +GLOBAL +DEFAULT +UND __tls_get_addr
[0-9 ]+: [0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +13 __bss_start
[0-9 ]+: 0+ +0 +TLS +GLOBAL +DEFAULT +UND sG1
[0-9 ]+: [0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +13 _edata
[0-9 ]+: [0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +13 _end

Symbol table '\.symtab' contains [0-9]+ entries:
 +Num: +Value +Size +Type +Bind +Vis +Ndx +Name
[0-9 ]+: [0-9a-f]+ +0 +NOTYPE +LOCAL +DEFAULT +UND 
[0-9 ]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +1 
[0-9 ]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +2 
[0-9 ]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +3 
[0-9 ]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +4 
[0-9 ]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +5 
[0-9 ]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +6 
[0-9 ]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +7 
[0-9 ]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +8 
[0-9 ]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +9 
[0-9 ]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +10 
[0-9 ]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +11 
[0-9 ]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +12 
[0-9 ]+: [0-9a-f]+ +0 +SECTION +LOCAL +DEFAULT +13 
.* FILE +LOCAL +DEFAULT +ABS .*
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +9 sl1
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +9 sl2
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +9 sl3
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +9 sl4
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +9 sl5
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +9 sl6
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +9 sl7
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +9 sl8
.* FILE +LOCAL +DEFAULT +ABS .*
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +10 bl1
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +10 bl2
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +10 bl3
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +10 bl4
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +10 bl5
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +10 bl6
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +10 bl7
[0-9 ]+: [0-9a-f]+ +0 +TLS +LOCAL +DEFAULT +10 bl8
.* FILE +LOCAL +DEFAULT +ABS .*
[0-9 ]+: [0-9a-f]+ +0 +OBJECT +LOCAL +DEFAULT +11 _DYNAMIC
[0-9 ]+: [0-9a-f]+ +0 +OBJECT +LOCAL +DEFAULT +12 _PROCEDURE_LINKAGE_TABLE_
[0-9 ]+: [0-9a-f]+ +0 +OBJECT +LOCAL +DEFAULT +13 _GLOBAL_OFFSET_TABLE_
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +9 sg8
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +10 bg8
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +10 bg6
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +10 bg3
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +9 sg3
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +HIDDEN +9 sh3
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +UND sG2
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +9 sg4
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +9 sg5
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +10 bg5
[0-9 ]+: [0-9a-f]+ +0 +FUNC +GLOBAL +DEFAULT +UND __tls_get_addr
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +HIDDEN +9 sh7
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +HIDDEN +9 sh8
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +9 sg1
[0-9 ]+: [0-9a-f]+ +52 +FUNC +GLOBAL +DEFAULT +7 _start
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +HIDDEN +9 sh4
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +10 bg7
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +HIDDEN +9 sh5
[0-9 ]+: [0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +13 __bss_start
[0-9 ]+: [0-9a-f]+ +136 +FUNC +GLOBAL +DEFAULT +\[<other>: 88\] +7 fn2
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +9 sg2
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +UND sG1
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +HIDDEN +9 sh1
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +9 sg6
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +9 sg7
[0-9 ]+: [0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +13 _edata
[0-9 ]+: [0-9a-f]+ +0 +NOTYPE +GLOBAL +DEFAULT +13 _end
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +HIDDEN +9 sh2
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +HIDDEN +9 sh6
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +10 bg2
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +10 bg1
[0-9 ]+: [0-9a-f]+ +0 +TLS +GLOBAL +DEFAULT +10 bg4
