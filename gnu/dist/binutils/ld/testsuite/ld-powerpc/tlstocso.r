#source: tlstoc.s
#as: -a64
#ld: -shared -melf64ppc
#readelf: -WSsrl
#target: powerpc64*-*-*

There are 22 section headers.*

Section Headers:
 +\[Nr\] Name +Type +Address +Off +Size +ES Flg Lk Inf Al
 +\[ 0\] +NULL +0+ 0+ 0+ 0+ +0 +0 +0
 +\[ 1\] \.hash +HASH +0+120 0+120 0+d8 04 +A +2 +0 +8
 +\[ 2\] \.dynsym +DYNSYM +0+1f8 0+1f8 0+348 18 +A +3 +13 +8
 +\[ 3\] \.dynstr +STRTAB +0+540 0+540 0+54 0+ +A +0 +0 +1
 +\[ 4\] \.rela\.dyn +RELA +0+598 0+598 0+108 18 +A +2 +0 +8
 +\[ 5\] \.rela\.plt +RELA +0+6a0 0+6a0 0+18 18 +A +2 +11 +8
 +\[ 6\] \.text +PROGBITS +0+6b8 0+6b8 0+9c 0+ +AX +0 +0 +4
 +\[ 7\] \.data +PROGBITS +0+10758 0+758 0+ 0+ +WA +0 +0 +1
 +\[ 8\] \.branch_lt +PROGBITS +0+10758 0+758 0+ 0+ +WA +0 +0 +8
 +\[ 9\] \.tdata +PROGBITS +0+10758 0+758 0+38 0+ WAT +0 +0 +8
 +\[10\] \.tbss +NOBITS +0+10790 0+790 0+38 0+ WAT +0 +0 +8
 +\[11\] \.dynamic +DYNAMIC +0+10790 0+790 0+150 10 +WA +3 +0 +8
 +\[12\] \.ctors +PROGBITS +0+108e0 0+938 0+ 0+ +W +0 +0 +1
 +\[13\] \.dtors +PROGBITS +0+108e0 0+938 0+ 0+ +W +0 +0 +1
 +\[14\] \.got +PROGBITS +0+108e0 0+8e0 0+8 08 +WA +0 +0 +8
 +\[15\] \.toc +PROGBITS +0+108e8 0+8e8 0+50 0+ +WA +0 +0 +1
 +\[16\] \.sbss +PROGBITS +0+10938 0+938 0+ 0+ +W +0 +0 +1
 +\[17\] \.plt +NOBITS +0+10938 0+938 0+30 18 +WA +0 +0 +8
 +\[18\] \.bss +NOBITS +0+10968 0+938 0+ 0+ +WA +0 +0 +1
 +\[19\] \.shstrtab +STRTAB +0+ 0+938 0+95 0+ +0 +0 +1
 +\[20\] \.symtab +SYMTAB +0+ 0+f50 0+468 18 +21 +1f +8
 +\[21\] \.strtab +STRTAB +0+ 0+13b8 0+92 0+ +0 +0 +1
#...

Elf file type is DYN \(Shared object file\)
Entry point 0x6d4
There are 4 program headers.*

Program Headers:
 +Type +Offset +VirtAddr +PhysAddr +FileSiz +MemSiz +Flg Align
 +LOAD +0x0+ 0x0+ 0x0+ 0x0+754 0x0+754 R E 0x10000
 +LOAD +0x0+758 0x0+10758 0x0+10758 0x0+1e0 0x0+210 RW +0x10000
 +DYNAMIC +0x0+790 0x0+10790 0x0+10790 0x0+150 0x0+150 RW +0x8
 +TLS +0x0+758 0x0+10758 0x0+10758 0x0+38 0x0+70 R +0x8

 Section to Segment mapping:
 +Segment Sections\.\.\.
 +0+ +\.hash \.dynsym \.dynstr \.rela\.dyn \.rela\.plt \.text 
 +01 +\.tdata \.tbss \.dynamic \.got \.toc \.plt 
 +02 +\.tbss \.dynamic 
 +03 +\.tdata \.tbss 

Relocation section '\.rela\.dyn' at offset .* contains 11 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+712 +0+f00000048 R_PPC64_TPREL16_HA +0+108e8 \.toc \+ 40
0+722 +0+1500000045 R_PPC64_TPREL16 +0+60 le0 \+ 0
0+726 +0+1800000048 R_PPC64_TPREL16_HA +0+68 le1 \+ 0
0+108e8 +0+1400000044 R_PPC64_DTPMOD64 +0+ gd \+ 0
0+108f0 +0+140000004e R_PPC64_DTPREL64 +0+ gd \+ 0
0+108f8 +0+1900000044 R_PPC64_DTPMOD64 +0+ ld \+ 0
0+10908 +0+2100000044 R_PPC64_DTPMOD64 +0+38 gd0 \+ 0
0+10910 +0+210000004e R_PPC64_DTPREL64 +0+38 gd0 \+ 0
0+10918 +0+1700000044 R_PPC64_DTPMOD64 +0+40 ld0 \+ 0
0+10928 +0+1c0000004e R_PPC64_DTPREL64 +0+50 ld2 \+ 0
0+10930 +0+2200000049 R_PPC64_TPREL64 +0+58 ie0 \+ 0

Relocation section '\.rela\.plt' at offset 0x6a0 contains 1 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+10950 +0+1600000015 R_PPC64_JMP_SLOT +0+ __tls_get_addr \+ 0

Symbol table '\.dynsym' contains 35 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +0: 0+ +0 NOTYPE +LOCAL +DEFAULT +UND 
 +1: 0+120 +0 SECTION LOCAL +DEFAULT +1 
 +2: 0+1f8 +0 SECTION LOCAL +DEFAULT +2 
 +3: 0+540 +0 SECTION LOCAL +DEFAULT +3 
 +4: 0+598 +0 SECTION LOCAL +DEFAULT +4 
 +5: 0+6a0 +0 SECTION LOCAL +DEFAULT +5 
 +6: 0+6b8 +0 SECTION LOCAL +DEFAULT +6 
 +7: 0+10758 +0 SECTION LOCAL +DEFAULT +7 
 +8: 0+10758 +0 SECTION LOCAL +DEFAULT +8 
 +9: 0+10758 +0 SECTION LOCAL +DEFAULT +9 
 +10: 0+10790 +0 SECTION LOCAL +DEFAULT +10 
 +11: 0+10790 +0 SECTION LOCAL +DEFAULT +11 
 +12: 0+108e0 +0 SECTION LOCAL +DEFAULT +12 
 +13: 0+108e0 +0 SECTION LOCAL +DEFAULT +13 
 +14: 0+108e0 +0 SECTION LOCAL +DEFAULT +14 
 +15: 0+108e8 +0 SECTION LOCAL +DEFAULT +15 
 +16: 0+10938 +0 SECTION LOCAL +DEFAULT +16 
 +17: 0+10938 +0 SECTION LOCAL +DEFAULT +17 
 +18: 0+10968 +0 SECTION LOCAL +DEFAULT +18 
 +19: 0+10790 +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
 +20: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND gd
 +21: 0+60 +0 TLS +GLOBAL DEFAULT +10 le0
 +22: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND __tls_get_addr
 +23: 0+40 +0 TLS +GLOBAL DEFAULT +10 ld0
 +24: 0+68 +0 TLS +GLOBAL DEFAULT +10 le1
 +25: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND ld
 +26: 0+6d4 +0 NOTYPE +GLOBAL DEFAULT +6 _start
 +27: 0+10968 +0 NOTYPE +GLOBAL DEFAULT +ABS __end
 +28: 0+50 +0 TLS +GLOBAL DEFAULT +10 ld2
 +29: 0+48 +0 TLS +GLOBAL DEFAULT +10 ld1
 +30: 0+10938 +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
 +31: 0+10938 +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
 +32: 0+10968 +0 NOTYPE +GLOBAL DEFAULT +ABS _end
 +33: 0+38 +0 TLS +GLOBAL DEFAULT +10 gd0
 +34: 0+58 +0 TLS +GLOBAL DEFAULT +10 ie0

Symbol table '\.symtab' contains 47 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +0: 0+ +0 NOTYPE +LOCAL +DEFAULT +UND 
 +1: 0+120 +0 SECTION LOCAL +DEFAULT +1 
 +2: 0+1f8 +0 SECTION LOCAL +DEFAULT +2 
 +3: 0+540 +0 SECTION LOCAL +DEFAULT +3 
 +4: 0+598 +0 SECTION LOCAL +DEFAULT +4 
 +5: 0+6a0 +0 SECTION LOCAL +DEFAULT +5 
 +6: 0+6b8 +0 SECTION LOCAL +DEFAULT +6 
 +7: 0+10758 +0 SECTION LOCAL +DEFAULT +7 
 +8: 0+10758 +0 SECTION LOCAL +DEFAULT +8 
 +9: 0+10758 +0 SECTION LOCAL +DEFAULT +9 
 +10: 0+10790 +0 SECTION LOCAL +DEFAULT +10 
 +11: 0+10790 +0 SECTION LOCAL +DEFAULT +11 
 +12: 0+108e0 +0 SECTION LOCAL +DEFAULT +12 
 +13: 0+108e0 +0 SECTION LOCAL +DEFAULT +13 
 +14: 0+108e0 +0 SECTION LOCAL +DEFAULT +14 
 +15: 0+108e8 +0 SECTION LOCAL +DEFAULT +15 
 +16: 0+10938 +0 SECTION LOCAL +DEFAULT +16 
 +17: 0+10938 +0 SECTION LOCAL +DEFAULT +17 
 +18: 0+10968 +0 SECTION LOCAL +DEFAULT +18 
 +19: 0+ +0 SECTION LOCAL +DEFAULT +19 
 +20: 0+ +0 SECTION LOCAL +DEFAULT +20 
 +21: 0+ +0 SECTION LOCAL +DEFAULT +21 
 +22: 0+ +0 TLS +LOCAL +DEFAULT +9 gd4
 +23: 0+8 +0 TLS +LOCAL +DEFAULT +9 ld4
 +24: 0+10 +0 TLS +LOCAL +DEFAULT +9 ld5
 +25: 0+18 +0 TLS +LOCAL +DEFAULT +9 ld6
 +26: 0+20 +0 TLS +LOCAL +DEFAULT +9 ie4
 +27: 0+28 +0 TLS +LOCAL +DEFAULT +9 le4
 +28: 0+30 +0 TLS +LOCAL +DEFAULT +9 le5
 +29: 0+10930 +0 NOTYPE +LOCAL +DEFAULT +15 \.Lie0
 +30: 0+6b8 +0 NOTYPE +LOCAL +DEFAULT +6 \.__tls_get_addr
 +31: 0+10790 +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
 +32: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND gd
 +33: 0+60 +0 TLS +GLOBAL DEFAULT +10 le0
 +34: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND __tls_get_addr
 +35: 0+40 +0 TLS +GLOBAL DEFAULT +10 ld0
 +36: 0+68 +0 TLS +GLOBAL DEFAULT +10 le1
 +37: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND ld
 +38: 0+6d4 +0 NOTYPE +GLOBAL DEFAULT +6 _start
 +39: 0+10968 +0 NOTYPE +GLOBAL DEFAULT +ABS __end
 +40: 0+50 +0 TLS +GLOBAL DEFAULT +10 ld2
 +41: 0+48 +0 TLS +GLOBAL DEFAULT +10 ld1
 +42: 0+10938 +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
 +43: 0+10938 +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
 +44: 0+10968 +0 NOTYPE +GLOBAL DEFAULT +ABS _end
 +45: 0+38 +0 TLS +GLOBAL DEFAULT +10 gd0
 +46: 0+58 +0 TLS +GLOBAL DEFAULT +10 ie0
