#source: tlslib.s
#source: tlstoc.s
#as: -a64
#ld: -melf64ppc
#readelf: -WSsrl
#target: powerpc64*-*-*

There are 23 section headers.*

Section Headers:
 +\[Nr\] Name +Type +Address +Off +Size +ES Flg Lk Inf Al
 +\[ 0\] +NULL +0+ 0+ 0+ 0+ +0 +0 +0
 +\[ 1\] \.interp +PROGBITS +0+10000190 0+190 0+11 0+ +A +0 +0 +1
 +\[ 2\] \.hash +HASH +0+100001a8 0+1a8 0+3c 04 +A +3 +0 +8
 +\[ 3\] \.dynsym +DYNSYM +0+100001e8 0+1e8 0+f0 18 +A +4 +1 +8
 +\[ 4\] \.dynstr +STRTAB +0+100002d8 0+2d8 0+4d 0+ +A +0 +0 +1
 +\[ 5\] \.rela\.dyn +RELA +0+10000328 0+328 0+30 18 +A +3 +0 +8
 +\[ 6\] \.rela\.plt +RELA +0+10000358 0+358 0+18 18 +A +3 +12 +8
 +\[ 7\] \.text +PROGBITS +0+10000370 0+370 0+9c 0+ +AX +0 +0 +4
 +\[ 8\] \.data +PROGBITS +0+10010410 0+410 0+ 0+ +WA +0 +0 +1
 +\[ 9\] \.branch_lt +PROGBITS +0+10010410 0+410 0+ 0+ +WA +0 +0 +8
 +\[10\] \.tdata +PROGBITS +0+10010410 0+410 0+38 0+ WAT +0 +0 +8
 +\[11\] \.tbss +NOBITS +0+10010448 0+448 0+38 0+ WAT +0 +0 +8
 +\[12\] \.dynamic +DYNAMIC +0+10010448 0+448 0+150 10 +WA +4 +0 +8
 +\[13\] \.ctors +PROGBITS +0+10010598 0+5f0 0+ 0+ +W +0 +0 +1
 +\[14\] \.dtors +PROGBITS +0+10010598 0+5f0 0+ 0+ +W +0 +0 +1
 +\[15\] \.got +PROGBITS +0+10010598 0+598 0+8 08 +WA +0 +0 +8
 +\[16\] \.toc +PROGBITS +0+100105a0 0+5a0 0+50 0+ +WA +0 +0 +1
 +\[17\] \.sbss +PROGBITS +0+100105f0 0+5f0 0+ 0+ +W +0 +0 +1
 +\[18\] \.plt +NOBITS +0+100105f0 0+5f0 0+30 18 +WA +0 +0 +8
 +\[19\] \.bss +NOBITS +0+10010620 0+5f0 0+ 0+ +WA +0 +0 +1
 +\[20\] \.shstrtab +STRTAB +0+ 0+5f0 0+9d 0+ +0 +0 +1
 +\[21\] \.symtab +SYMTAB +0+ 0+c50 0+480 18 +22 +1f +8
 +\[22\] \.strtab +STRTAB +0+ 0+10d0 0+92 0+ +0 +0 +1
#...

Elf file type is EXEC \(Executable file\)
Entry point 0x1000038c
There are 6 program headers.*

Program Headers:
 +Type +Offset +VirtAddr +PhysAddr +FileSiz +MemSiz +Flg Align
 +PHDR +0x0+40 0x0+10000040 0x0+10000040 0x0+150 0x0+150 R E 0x8
 +INTERP +0x0+190 0x0+10000190 0x0+10000190 0x0+11 0x0+11 R +0x1
 +\[Requesting program interpreter: .*\]
 +LOAD +0x0+ 0x0+10000000 0x0+10000000 0x0+40c 0x0+40c R E 0x10000
 +LOAD +0x0+410 0x0+10010410 0x0+10010410 0x0+1e0 0x0+210 RW +0x10000
 +DYNAMIC +0x0+448 0x0+10010448 0x0+10010448 0x0+150 0x0+150 RW +0x8
 +TLS +0x0+410 0x0+10010410 0x0+10010410 0x0+38 0x0+70 R +0x8

 Section to Segment mapping:
 +Segment Sections\.\.\.
 +0+ +
 +01 +\.interp 
 +02 +\.interp \.hash \.dynsym \.dynstr \.rela\.dyn \.rela\.plt \.text 
 +03 +\.tdata \.tbss \.dynamic \.got \.toc \.plt 
 +04 +\.tbss \.dynamic 
 +05 +\.tdata \.tbss 

Relocation section '\.rela\.dyn' at offset .* contains 2 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+100105a0 +0+200000049 R_PPC64_TPREL64 +0+ gd \+ 0
0+100105b0 +0+500000044 R_PPC64_DTPMOD64 +0+ ld \+ 0

Relocation section '\.rela\.plt' at offset .* contains 1 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+10010608 +0+300000015 R_PPC64_JMP_SLOT +0+ __tls_get_addr \+ 0

Symbol table '\.dynsym' contains 10 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +0: 0+ +0 NOTYPE +LOCAL +DEFAULT +UND 
 +1: 0+10010448 +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
 +2: 0+ +0 TLS +GLOBAL DEFAULT +UND gd
 +3: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND __tls_get_addr
 +4: 0+ +0 FUNC +GLOBAL DEFAULT +UND \.__tls_get_addr
 +5: 0+ +0 TLS +GLOBAL DEFAULT +UND ld
 +6: 0+10010620 +0 NOTYPE +GLOBAL DEFAULT +ABS __end
 +7: 0+100105f0 +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
 +8: 0+100105f0 +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
 +9: 0+10010620 +0 NOTYPE +GLOBAL DEFAULT +ABS _end

Symbol table '\.symtab' contains 48 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +0: 0+ +0 NOTYPE +LOCAL +DEFAULT +UND 
 +1: 0+10000190 +0 SECTION LOCAL +DEFAULT +1 
 +2: 0+100001a8 +0 SECTION LOCAL +DEFAULT +2 
 +3: 0+100001e8 +0 SECTION LOCAL +DEFAULT +3 
 +4: 0+100002d8 +0 SECTION LOCAL +DEFAULT +4 
 +5: 0+10000328 +0 SECTION LOCAL +DEFAULT +5 
 +6: 0+10000358 +0 SECTION LOCAL +DEFAULT +6 
 +7: 0+10000370 +0 SECTION LOCAL +DEFAULT +7 
 +8: 0+10010410 +0 SECTION LOCAL +DEFAULT +8 
 +9: 0+10010410 +0 SECTION LOCAL +DEFAULT +9 
 +10: 0+10010410 +0 SECTION LOCAL +DEFAULT +10 
 +11: 0+10010448 +0 SECTION LOCAL +DEFAULT +11 
 +12: 0+10010448 +0 SECTION LOCAL +DEFAULT +12 
 +13: 0+10010598 +0 SECTION LOCAL +DEFAULT +13 
 +14: 0+10010598 +0 SECTION LOCAL +DEFAULT +14 
 +15: 0+10010598 +0 SECTION LOCAL +DEFAULT +15 
 +16: 0+100105a0 +0 SECTION LOCAL +DEFAULT +16 
 +17: 0+100105f0 +0 SECTION LOCAL +DEFAULT +17 
 +18: 0+100105f0 +0 SECTION LOCAL +DEFAULT +18 
 +19: 0+10010620 +0 SECTION LOCAL +DEFAULT +19 
 +20: 0+ +0 SECTION LOCAL +DEFAULT +20 
 +21: 0+ +0 SECTION LOCAL +DEFAULT +21 
 +22: 0+ +0 SECTION LOCAL +DEFAULT +22 
 +23: 0+ +0 TLS +LOCAL +DEFAULT +10 gd4
 +24: 0+8 +0 TLS +LOCAL +DEFAULT +10 ld4
 +25: 0+10 +0 TLS +LOCAL +DEFAULT +10 ld5
 +26: 0+18 +0 TLS +LOCAL +DEFAULT +10 ld6
 +27: 0+20 +0 TLS +LOCAL +DEFAULT +10 ie4
 +28: 0+28 +0 TLS +LOCAL +DEFAULT +10 le4
 +29: 0+30 +0 TLS +LOCAL +DEFAULT +10 le5
 +30: 0+100105e8 +0 NOTYPE +LOCAL +DEFAULT +16 \.Lie0
 +31: 0+10010448 +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
 +32: 0+ +0 TLS +GLOBAL DEFAULT +UND gd
 +33: 0+60 +0 TLS +GLOBAL DEFAULT +11 le0
 +34: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND __tls_get_addr
 +35: 0+ +0 FUNC +GLOBAL DEFAULT +UND \.__tls_get_addr
 +36: 0+40 +0 TLS +GLOBAL DEFAULT +11 ld0
 +37: 0+68 +0 TLS +GLOBAL DEFAULT +11 le1
 +38: 0+ +0 TLS +GLOBAL DEFAULT +UND ld
 +39: 0+1000038c +0 NOTYPE +GLOBAL DEFAULT +7 _start
 +40: 0+10010620 +0 NOTYPE +GLOBAL DEFAULT +ABS __end
 +41: 0+50 +0 TLS +GLOBAL DEFAULT +11 ld2
 +42: 0+48 +0 TLS +GLOBAL DEFAULT +11 ld1
 +43: 0+100105f0 +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
 +44: 0+100105f0 +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
 +45: 0+10010620 +0 NOTYPE +GLOBAL DEFAULT +ABS _end
 +46: 0+38 +0 TLS +GLOBAL DEFAULT +11 gd0
 +47: 0+58 +0 TLS +GLOBAL DEFAULT +11 ie0
