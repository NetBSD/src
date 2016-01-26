#source: tls32.s
#source: tlslib32.s
#as: -a32
#ld: 
#readelf: -WSsrl
#target: powerpc*-*-*

There are [0-9]+ section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
 +\[Nr\] Name +Type +Addr +Off +Size +ES Flg Lk Inf Al
 +\[[ 0-9]+\] +NULL +0+ 0+ 0+ 00 +0 +0 +0
 +\[[ 0-9]+\] \.interp +.*
 +\[[ 0-9]+\] \.hash +.*
 +\[[ 0-9]+\] \.dynsym +.*
 +\[[ 0-9]+\] \.dynstr +.*
 +\[[ 0-9]+\] \.rela\.dyn +.*
 +\[[ 0-9]+\] \.rela\.plt +.*
 +\[[ 0-9]+\] \.text +PROGBITS +[0-9a-f]+ [0-9a-f]+ 000070 00 +AX +0 +0 +1
 +\[[ 0-9]+\] \.tdata +PROGBITS +[0-9a-f]+ [0-9a-f]+ 00001c 00 WAT +0 +0 +4
 +\[[ 0-9]+\] \.tbss +NOBITS +[0-9a-f]+ [0-9a-f]+ 00001c 00 WAT +0 +0 +4
 +\[[ 0-9]+\] \.dynamic +DYNAMIC +[0-9a-f]+ [0-9a-f]+ [0-9a-f]+ 08 +WA +4 +0 +4
 +\[[ 0-9]+\] \.got +PROGBITS +[0-9a-f]+ [0-9a-f]+ 00001c 04 WAX +0 +0 +4
 +\[[ 0-9]+\] \.plt +NOBITS +.*
 +\[[ 0-9]+\] \.shstrtab +STRTAB +.*
 +\[[ 0-9]+\] \.symtab +SYMTAB +.*
 +\[[ 0-9]+\] \.strtab +STRTAB +.*
#...

Elf file type is EXEC \(Executable file\)
Entry point .*
There are [0-9]+ program headers, starting at offset [0-9]+

Program Headers:
 +Type +Offset +VirtAddr +PhysAddr +FileSiz MemSiz +Flg Align
 +PHDR +0x000034 0x01800034 0x01800034 0x000c0 0x000c0 R E 0x4
 +INTERP +0x0000f4 0x018000f4 0x018000f4 0x00011 0x00011 R +0x1
 +\[Requesting program interpreter: .*\]
 +LOAD .* R E 0x10000
 +LOAD .* RWE 0x10000
 +DYNAMIC .* RW +0x4
 +TLS .* 0x0001c 0x00038 R +0x4

 Section to Segment mapping:
 +Segment Sections\.\.\.
 +00 +
 +01 +\.interp 
 +02 +\.interp \.hash \.dynsym \.dynstr \.rela\.dyn \.rela\.plt \.text 
 +03 +\.tdata \.dynamic \.got \.plt 
 +04 +\.dynamic 
 +05 +\.tdata \.tbss 

Relocation section '\.rela\.dyn' at offset .* contains 2 entries:
 Offset +Info +Type +Sym\. Value +Symbol's Name \+ Addend
[0-9a-f ]+R_PPC_TPREL32 +00000000 +gd \+ 0
[0-9a-f ]+R_PPC_DTPMOD32 +00000000 +ld \+ 0

Relocation section '\.rela\.plt' at offset .* contains 1 entries:
 Offset +Info +Type +Sym\. Value +Symbol's Name \+ Addend
[0-9a-f ]+R_PPC_JMP_SLOT[0-9a-f ]+__tls_get_addr_opt \+ 0

Symbol table '\.dynsym' contains [0-9]+ entries:
 +Num: +Value +Size +Type +Bind +Vis +Ndx +Name
.* NOTYPE +LOCAL +DEFAULT +UND 
.* TLS +GLOBAL +DEFAULT +UND gd
.* TLS +GLOBAL +DEFAULT +UND ld
.* NOTYPE +GLOBAL +DEFAULT +12 __end
.* NOTYPE +GLOBAL +DEFAULT +12 __bss_start
.* FUNC +GLOBAL +DEFAULT +UND __tls_get_addr_opt
.* NOTYPE +GLOBAL +DEFAULT +11 _edata
.* NOTYPE +GLOBAL +DEFAULT +12 _end

Symbol table '\.symtab' contains [0-9]+ entries:
 +Num: +Value +Size +Type +Bind +Vis +Ndx +Name
.* NOTYPE +LOCAL +DEFAULT +UND 
.* SECTION +LOCAL +DEFAULT +1 
.* SECTION +LOCAL +DEFAULT +2 
.* SECTION +LOCAL +DEFAULT +3 
.* SECTION +LOCAL +DEFAULT +4 
.* SECTION +LOCAL +DEFAULT +5 
.* SECTION +LOCAL +DEFAULT +6 
.* SECTION +LOCAL +DEFAULT +7 
.* SECTION +LOCAL +DEFAULT +8 
.* SECTION +LOCAL +DEFAULT +9 
.* SECTION +LOCAL +DEFAULT +10 
.* SECTION +LOCAL +DEFAULT +11 
.* SECTION +LOCAL +DEFAULT +12 
.* FILE +LOCAL +DEFAULT +ABS .*
.* TLS +LOCAL +DEFAULT +8 gd4
.* TLS +LOCAL +DEFAULT +8 ld4
.* TLS +LOCAL +DEFAULT +8 ld5
.* TLS +LOCAL +DEFAULT +8 ld6
.* TLS +LOCAL +DEFAULT +8 ie4
.* TLS +LOCAL +DEFAULT +8 le4
.* TLS +LOCAL +DEFAULT +8 le5
.* FILE +LOCAL +DEFAULT +ABS .*
.* OBJECT +LOCAL +DEFAULT +10 _DYNAMIC
.* OBJECT +LOCAL +DEFAULT +11 _GLOBAL_OFFSET_TABLE_
.* TLS +GLOBAL +DEFAULT +UND gd
.* TLS +GLOBAL +DEFAULT +9 le0
.* TLS +GLOBAL +DEFAULT +9 ld0
.* TLS +GLOBAL +DEFAULT +9 le1
.* TLS +GLOBAL +DEFAULT +UND ld
.* NOTYPE +GLOBAL +DEFAULT +7 _start
.* NOTYPE +GLOBAL +DEFAULT +12 __end
.* TLS +GLOBAL +DEFAULT +9 ld2
.* TLS +GLOBAL +DEFAULT +9 ld1
.* NOTYPE +GLOBAL +DEFAULT +12 __bss_start
.* FUNC +GLOBAL +DEFAULT +UND __tls_get_addr_opt
.* NOTYPE +GLOBAL +DEFAULT +11 _edata
.* NOTYPE +GLOBAL +DEFAULT +12 _end
.* TLS +GLOBAL +DEFAULT +9 gd0
.* TLS +GLOBAL +DEFAULT +9 ie0
