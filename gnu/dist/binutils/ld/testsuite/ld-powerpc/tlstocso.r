#source: tlstoc.s
#as: -a64
#ld: -shared -melf64ppc
#readelf: -WSsrl
#target: powerpc64*-*-*

There are 19 section headers.*

Section Headers:
 +\[Nr\] Name +Type +Address +Off +Size +ES Flg Lk Inf Al
 +\[ 0\] +NULL +0+ 0+ 0+ 0+ +0 +0 +0
 +\[ 1\] \.hash +.*
 +\[ 2\] \.dynsym +.*
 +\[ 3\] \.dynstr +.*
 +\[ 4\] \.rela\.dyn +.*
 +\[ 5\] \.rela\.plt +.*
 +\[ 6\] \.text +PROGBITS +0+550 0+550 0+bc 0+ +AX +0 +0 +4
 +\[ 7\] \.tdata +PROGBITS +0+10610 0+610 0+38 0+ WAT +0 +0 +8
 +\[ 8\] \.tbss +NOBITS +0+10648 0+648 0+38 0+ WAT +0 +0 +8
 +\[ 9\] \.dynamic +DYNAMIC +0+10648 0+648 0+150 10 +WA +3 +0 +8
 +\[10\] \.data +PROGBITS +0+10798 0+798 0+ 0+ +WA +0 +0 +1
 +\[11\] \.branch_lt +.*
 +\[12\] \.got +PROGBITS +0+10798 0+798 0+58 08 +WA +0 +0 +8
 +\[13\] \.sbss +.*
 +\[14\] \.plt +.*
 +\[15\] \.bss +.*
 +\[16\] \.shstrtab +.*
 +\[17\] \.symtab +.*
 +\[18\] \.strtab +.*
#...

Elf file type is DYN \(Shared object file\)
Entry point 0x[0-9a-f]+
There are 4 program headers.*

Program Headers:
 +Type +Offset +VirtAddr +PhysAddr +FileSiz +MemSiz +Flg Align
 +LOAD +0x0+ 0x0+ 0x0+ 0x0+60c 0x0+60c R E 0x10000
 +LOAD +0x0+610 0x0+10610 0x0+10610 0x0+1e0 0x0+210 RW +0x10000
 +DYNAMIC +0x0+648 0x0+10648 0x0+10648 0x0+150 0x0+150 RW +0x8
 +TLS +0x0+610 0x0+10610 0x0+10610 0x0+38 0x0+70 R +0x8

 Section to Segment mapping:
 +Segment Sections\.\.\.
 +0+ +\.hash \.dynsym \.dynstr \.rela\.dyn \.rela\.plt \.text 
 +01 +\.tdata \.dynamic \.got \.plt 
 +02 +\.dynamic 
 +03 +\.tdata \.tbss 

Relocation section '\.rela\.dyn' at offset .* contains 11 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+5ba +0+90+45 R_PPC64_TPREL16 +0+60 le0 \+ 0
0+5be +0+c0+48 R_PPC64_TPREL16_HA +0+68 le1 \+ 0
0+5c2 +0+c0+46 R_PPC64_TPREL16_LO +0+68 le1 \+ 0
0+107a0 +0+80+44 R_PPC64_DTPMOD64 +0+ gd \+ 0
0+107a8 +0+80+4e R_PPC64_DTPREL64 +0+ gd \+ 0
0+107b0 +0+d0+44 R_PPC64_DTPMOD64 +0+ ld \+ 0
0+107c0 +0+140+44 R_PPC64_DTPMOD64 +0+38 gd0 \+ 0
0+107c8 +0+140+4e R_PPC64_DTPREL64 +0+38 gd0 \+ 0
0+107d0 +0+b0+44 R_PPC64_DTPMOD64 +0+40 ld0 \+ 0
0+107e0 +0+f0+4e R_PPC64_DTPREL64 +0+50 ld2 \+ 0
0+107e8 +0+150+49 R_PPC64_TPREL64 +0+58 ie0 \+ 0

Relocation section '\.rela\.plt' at offset .* contains 1 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+10808 +0+a0+15 R_PPC64_JMP_SLOT +0+ __tls_get_addr \+ 0

Symbol table '\.dynsym' contains 22 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +[0-9]+: 0+ +0 NOTYPE +LOCAL +DEFAULT +UND 
 +[0-9]+: 0+550 +0 SECTION LOCAL +DEFAULT +6 
 +[0-9]+: 0+10610 +0 SECTION LOCAL +DEFAULT +7 
 +[0-9]+: 0+10648 +0 SECTION LOCAL +DEFAULT +8 
 +[0-9]+: 0+10798 +0 SECTION LOCAL +DEFAULT +10 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +11 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +15 
 +[0-9]+: 0+10648 +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
 +[0-9]+: 0+ +0 TLS +GLOBAL DEFAULT +UND gd
 +[0-9]+: 0+60 +0 TLS +GLOBAL DEFAULT +8 le0
 +[0-9]+: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND __tls_get_addr
 +[0-9]+: 0+40 +0 TLS +GLOBAL DEFAULT +8 ld0
 +[0-9]+: 0+68 +0 TLS +GLOBAL DEFAULT +8 le1
 +[0-9]+: 0+ +0 TLS +GLOBAL DEFAULT +UND ld
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +6 _start
 +[0-9]+: 0+50 +0 TLS +GLOBAL DEFAULT +8 ld2
 +[0-9]+: 0+48 +0 TLS +GLOBAL DEFAULT +8 ld1
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS _end
 +[0-9]+: 0+38 +0 TLS +GLOBAL DEFAULT +8 gd0
 +[0-9]+: 0+58 +0 TLS +GLOBAL DEFAULT +8 ie0

Symbol table '\.symtab' contains 43 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +[0-9]+: 0+ +0 NOTYPE +LOCAL +DEFAULT +UND 
 +[0-9]+: 0+120 +0 SECTION LOCAL +DEFAULT +1 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +2 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +3 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +4 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +5 
 +[0-9]+: 0+550 +0 SECTION LOCAL +DEFAULT +6 
 +[0-9]+: 0+10610 +0 SECTION LOCAL +DEFAULT +7 
 +[0-9]+: 0+10648 +0 SECTION LOCAL +DEFAULT +8 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +9 
 +[0-9]+: 0+10798 +0 SECTION LOCAL +DEFAULT +10 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +11 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +12 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +13 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +14 
 +[0-9]+: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +15 
 +[0-9]+: 0+ +0 SECTION LOCAL +DEFAULT +16 
 +[0-9]+: 0+ +0 SECTION LOCAL +DEFAULT +17 
 +[0-9]+: 0+ +0 SECTION LOCAL +DEFAULT +18 
 +[0-9]+: 0+ +0 TLS +LOCAL +DEFAULT +7 gd4
 +[0-9]+: 0+8 +0 TLS +LOCAL +DEFAULT +7 ld4
 +[0-9]+: 0+10 +0 TLS +LOCAL +DEFAULT +7 ld5
 +[0-9]+: 0+18 +0 TLS +LOCAL +DEFAULT +7 ld6
 +[0-9]+: 0+20 +0 TLS +LOCAL +DEFAULT +7 ie4
 +[0-9]+: 0+28 +0 TLS +LOCAL +DEFAULT +7 le4
 +[0-9]+: 0+30 +0 TLS +LOCAL +DEFAULT +7 le5
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +LOCAL +DEFAULT +12 \.Lie0
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +LOCAL +DEFAULT +6 \.__tls_get_addr
 +[0-9]+: 0+10648 +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
 +[0-9]+: 0+ +0 TLS +GLOBAL DEFAULT +UND gd
 +[0-9]+: 0+60 +0 TLS +GLOBAL DEFAULT +8 le0
 +[0-9]+: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND __tls_get_addr
 +[0-9]+: 0+40 +0 TLS +GLOBAL DEFAULT +8 ld0
 +[0-9]+: 0+68 +0 TLS +GLOBAL DEFAULT +8 le1
 +[0-9]+: 0+ +0 TLS +GLOBAL DEFAULT +UND ld
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +6 _start
 +[0-9]+: 0+50 +0 TLS +GLOBAL DEFAULT +8 ld2
 +[0-9]+: 0+48 +0 TLS +GLOBAL DEFAULT +8 ld1
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
 +[0-9]+: [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS _end
 +[0-9]+: 0+38 +0 TLS +GLOBAL DEFAULT +8 gd0
 +[0-9]+: 0+58 +0 TLS +GLOBAL DEFAULT +8 ie0
