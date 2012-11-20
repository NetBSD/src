#source: tls.s
#source: tlslib.s
#as: -a64
#ld: -melf64ppc
#readelf: -WSsrl
#target: powerpc64*-*-*

There are 20 section headers.*

Section Headers:
 +\[Nr\] Name +Type +Address +Off +Size +ES Flg Lk Inf Al
 +\[ 0\] +NULL +0+ 0+ 0+ 0+ +0 +0 +0
 +\[ 1\] \.interp +.*
 +\[ 2\] \.hash +.*
 +\[ 3\] \.dynsym +.*
 +\[ 4\] \.dynstr +.*
 +\[ 5\] \.rela\.dyn +.*
 +\[ 6\] \.rela\.plt +.*
 +\[ 7\] \.text +PROGBITS .* 0+fc 0+ +AX +0 +0 +4
 +\[ 8\] \.tdata +PROGBITS .* 0+38 0+ WAT +0 +0 +8
 +\[ 9\] \.tbss +NOBITS .* 0+38 0+ WAT +0 +0 +8
 +\[10\] \.dynamic +DYNAMIC .* 0+150 10 +WA +4 +0 +8
 +\[11\] \.data +PROGBITS .* 0+ 0+ +WA +0 +0 +1
 +\[12\] \.branch_lt +.*
 +\[13\] \.got +PROGBITS .* 0+30 08 +WA +0 +0 +8
 +\[14\] \.sbss +.*
 +\[15\] \.plt +.*
 +\[16\] \.bss +.*
 +\[17\] \.shstrtab +.*
 +\[18\] \.symtab +.*
 +\[19\] \.strtab +.*
#...

Elf file type is EXEC \(Executable file\)
Entry point .*
There are 6 program headers.*

Program Headers:
 +Type +Offset +VirtAddr +PhysAddr +FileSiz +MemSiz +Flg Align
 +PHDR +0x0+40 0x0+10000040 0x0+10000040 0x0+150 0x0+150 R E 0x8
 +INTERP +0x0+190 0x0+10000190 0x0+10000190 0x0+11 0x0+11 R +0x1
 +\[Requesting program interpreter: .*\]
 +LOAD +0x0+ 0x0+10000000 0x0+10000000 0x0+464 0x0+464 R E 0x10000
 +LOAD +0x0+468 0x0+10010468 0x0+10010468 0x0+1b8 0x0+1e8 RW +0x10000
 +DYNAMIC +0x0+4a0 0x0+100104a0 0x0+100104a0 0x0+150 0x0+150 RW +0x8
 +TLS +0x0+468 0x0+10010468 0x0+10010468 0x0+38 0x0+70 R +0x8

 Section to Segment mapping:
 +Segment Sections\.\.\.
 +0+ +
 +01 +\.interp 
 +02 +\.interp \.hash \.dynsym \.dynstr \.rela\.dyn \.rela\.plt \.text 
 +03 +\.tdata \.dynamic \.got \.plt 
 +04 +\.dynamic 
 +05 +\.tdata \.tbss 

Relocation section '\.rela\.dyn' at offset .* contains 3 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+10010600 +0+200000049 R_PPC64_TPREL64 +0+ gd \+ 0
0+10010608 +0+400000044 R_PPC64_DTPMOD64 +0+ ld \+ 0
0+10010618 +0+50000004e R_PPC64_DTPREL64 +0+50 ld2 \+ 0

Relocation section '\.rela\.plt' at offset .* contains 1 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+10010638 +0+300000015 R_PPC64_JMP_SLOT +0+ __tls_get_addr \+ 0

Symbol table '\.dynsym' contains 9 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
.* 0+ +0 NOTYPE +LOCAL +DEFAULT +UND 
.* 0+100104a0 +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
.* 0+ +0 TLS +GLOBAL DEFAULT +UND gd
.* 0+ +0 FUNC +GLOBAL DEFAULT +UND __tls_get_addr
.* 0+ +0 TLS +GLOBAL DEFAULT +UND ld
.* 0+50 +0 TLS +GLOBAL DEFAULT +9 ld2
.* 0+10010620 +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
.* 0+10010620 +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
.* 0+10010650 +0 NOTYPE +GLOBAL DEFAULT +ABS _end

Symbol table '\.symtab' contains 43 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
.* 0+ +0 NOTYPE +LOCAL +DEFAULT +UND 
.* [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +1 
.* [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +2 
.* [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +3 
.* [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +4 
.* [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +5 
.* [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +6 
.* 0+10000368 +0 SECTION LOCAL +DEFAULT +7 
.* 0+10010468 +0 SECTION LOCAL +DEFAULT +8 
.* 0+100104a0 +0 SECTION LOCAL +DEFAULT +9 
.* 0+100104a0 +0 SECTION LOCAL +DEFAULT +10 
.* 0+100105f0 +0 SECTION LOCAL +DEFAULT +11 
.* [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +12 
.* 0+100105f0 +0 SECTION LOCAL +DEFAULT +13 
.* [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +14 
.* [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +15 
.* [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +16 
.* 0+ +0 SECTION LOCAL +DEFAULT +17 
.* 0+ +0 SECTION LOCAL +DEFAULT +18 
.* 0+ +0 SECTION LOCAL +DEFAULT +19 
.* 0+ +0 TLS +LOCAL +DEFAULT +8 gd4
.* 0+8 +0 TLS +LOCAL +DEFAULT +8 ld4
.* 0+10 +0 TLS +LOCAL +DEFAULT +8 ld5
.* 0+18 +0 TLS +LOCAL +DEFAULT +8 ld6
.* 0+20 +0 TLS +LOCAL +DEFAULT +8 ie4
.* 0+28 +0 TLS +LOCAL +DEFAULT +8 le4
.* 0+30 +0 TLS +LOCAL +DEFAULT +8 le5
.* 0+ +0 FUNC +LOCAL +DEFAULT +UND \.__tls_get_addr
.* 0+100104a0 +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
.* 0+ +0 TLS +GLOBAL DEFAULT +UND gd
.* 0+60 +0 TLS +GLOBAL DEFAULT +9 le0
.* 0+ +0 FUNC +GLOBAL DEFAULT +UND __tls_get_addr
.* 0+40 +0 TLS +GLOBAL DEFAULT +9 ld0
.* 0+68 +0 TLS +GLOBAL DEFAULT +9 le1
.* 0+ +0 TLS +GLOBAL DEFAULT +UND ld
.* [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +7 _start
.* 0+50 +0 TLS +GLOBAL DEFAULT +9 ld2
.* 0+48 +0 TLS +GLOBAL DEFAULT +9 ld1
.* [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
.* [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
.* [0-9a-f]+ +0 NOTYPE +GLOBAL DEFAULT +ABS _end
.* 0+38 +0 TLS +GLOBAL DEFAULT +9 gd0
.* 0+58 +0 TLS +GLOBAL DEFAULT +9 ie0
