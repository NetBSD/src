#source: tls.s
#as: -a64
#ld: -shared -melf64ppc
#readelf: -WSsrl
#target: powerpc64*-*-*

There are 21 section headers.*

Section Headers:
 +\[Nr\] Name +Type +Address +Off +Size +ES Flg Lk Inf Al
 +\[ 0\] +NULL +0+ 0+ 0+ 0+ +0 +0 +0
 +\[ 1\] \.hash +HASH +0+120 0+120 0+d4 04 +A +2 +0 +8
 +\[ 2\] \.dynsym +DYNSYM +0+1f8 0+1f8 0+330 18 +A +3 +12 +8
 +\[ 3\] \.dynstr +STRTAB +0+528 0+528 0+54 0+ +A +0 +0 +1
 +\[ 4\] \.rela\.dyn +RELA +0+580 0+580 0+180 18 +A +2 +0 +8
 +\[ 5\] \.rela\.plt +RELA +0+700 0+700 0+18 18 +A +2 +10 +8
 +\[ 6\] \.text +PROGBITS +0+718 0+718 0+dc 0+ +AX +0 +0 +4
 +\[ 7\] \.data +PROGBITS +0+107f8 0+7f8 0+ 0+ +WA +0 +0 +1
 +\[ 8\] \.branch_lt +PROGBITS +0+107f8 0+7f8 0+ 0+ +WA +0 +0 +8
 +\[ 9\] \.tdata +PROGBITS +0+107f8 0+7f8 0+38 0+ WAT +0 +0 +8
 +\[10\] \.tbss +NOBITS +0+10830 0+830 0+38 0+ WAT +0 +0 +8
 +\[11\] \.dynamic +DYNAMIC +0+10830 0+830 0+150 10 +WA +3 +0 +8
 +\[12\] \.ctors +PROGBITS +0+10980 0+9e0 0+ 0+ +W +0 +0 +1
 +\[13\] \.dtors +PROGBITS +0+10980 0+9e0 0+ 0+ +W +0 +0 +1
 +\[14\] \.got +PROGBITS +0+10980 0+980 0+60 08 +WA +0 +0 +8
 +\[15\] \.sbss +PROGBITS +0+109e0 0+9e0 0+ 0+ +W +0 +0 +1
 +\[16\] \.plt +NOBITS +0+109e0 0+9e0 0+30 18 +WA +0 +0 +8
 +\[17\] \.bss +NOBITS +0+10a10 0+9e0 0+ 0+ +WA +0 +0 +1
 +\[18\] \.shstrtab +STRTAB +0+ 0+9e0 0+90 0+ +0 +0 +1
 +\[19\] \.symtab +SYMTAB +0+ 0+fb0 0+438 18 +20 +1d +8
 +\[20\] \.strtab +STRTAB +0+ 0+13e8 0+8c 0+ +0 +0 +1
#...

Elf file type is DYN \(Shared object file\)
Entry point 0x734
There are 4 program headers.*

Program Headers:
 +Type +Offset +VirtAddr +PhysAddr +FileSiz +MemSiz +Flg Align
 +LOAD +0x0+ 0x0+ 0x0+ 0x0+7f4 0x0+7f4 R E 0x10000
 +LOAD +0x0+7f8 0x0+107f8 0x0+107f8 0x0+1e8 0x0+218 RW +0x10000
 +DYNAMIC +0x0+830 0x0+10830 0x0+10830 0x0+150 0x0+150 RW +0x8
 +TLS +0x0+7f8 0x0+107f8 0x0+107f8 0x0+38 0x0+70 R +0x8

 Section to Segment mapping:
 +Segment Sections\.\.\.
 +0+ +\.hash \.dynsym \.dynstr \.rela\.dyn \.rela\.plt \.text 
 +01 +\.tdata \.tbss \.dynamic \.got \.plt 
 +02 +\.tbss \.dynamic 
 +03 +\.tdata \.tbss 

Relocation section '\.rela\.dyn' at offset .* contains 16 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+782 +0+1400000045 R_PPC64_TPREL16 +0+60 le0 \+ 0
0+786 +0+1700000048 R_PPC64_TPREL16_HA +0+68 le1 \+ 0
0+78a +0+1700000046 R_PPC64_TPREL16_LO +0+68 le1 \+ 0
0+7c2 +0+90000005f R_PPC64_TPREL16_DS +0+107f8 \.tdata \+ 28
0+7c6 +0+900000048 R_PPC64_TPREL16_HA +0+107f8 \.tdata \+ 30
0+7ca +0+900000046 R_PPC64_TPREL16_LO +0+107f8 \.tdata \+ 30
0+10988 +0+44 R_PPC64_DTPMOD64 +0+
0+10998 +0+44 R_PPC64_DTPMOD64 +0+
0+109a0 +0+4e R_PPC64_DTPREL64 +0+
0+109a8 +0+4e R_PPC64_DTPREL64 +0+18
0+109b0 +0+1300000044 R_PPC64_DTPMOD64 +0+ gd \+ 0
0+109b8 +0+130000004e R_PPC64_DTPREL64 +0+ gd \+ 0
0+109c0 +0+1b0000004e R_PPC64_DTPREL64 +0+50 ld2 \+ 0
0+109c8 +0+2000000044 R_PPC64_DTPMOD64 +0+38 gd0 \+ 0
0+109d0 +0+200000004e R_PPC64_DTPREL64 +0+38 gd0 \+ 0
0+109d8 +0+2100000049 R_PPC64_TPREL64 +0+58 ie0 \+ 0

Relocation section '\.rela\.plt' at offset .* contains 1 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+109f8 +0+1500000015 R_PPC64_JMP_SLOT +0+ __tls_get_addr \+ 0

Symbol table '\.dynsym' contains 34 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +0: 0+ +0 NOTYPE +LOCAL +DEFAULT +UND 
 +1: 0+120 +0 SECTION LOCAL +DEFAULT +1 
 +2: 0+1f8 +0 SECTION LOCAL +DEFAULT +2 
 +3: 0+528 +0 SECTION LOCAL +DEFAULT +3 
 +4: 0+580 +0 SECTION LOCAL +DEFAULT +4 
 +5: 0+700 +0 SECTION LOCAL +DEFAULT +5 
 +6: 0+718 +0 SECTION LOCAL +DEFAULT +6 
 +7: 0+107f8 +0 SECTION LOCAL +DEFAULT +7 
 +8: 0+107f8 +0 SECTION LOCAL +DEFAULT +8 
 +9: 0+107f8 +0 SECTION LOCAL +DEFAULT +9 
 +10: 0+10830 +0 SECTION LOCAL +DEFAULT +10 
 +11: 0+10830 +0 SECTION LOCAL +DEFAULT +11 
 +12: 0+10980 +0 SECTION LOCAL +DEFAULT +12 
 +13: 0+10980 +0 SECTION LOCAL +DEFAULT +13 
 +14: 0+10980 +0 SECTION LOCAL +DEFAULT +14 
 +15: 0+109e0 +0 SECTION LOCAL +DEFAULT +15 
 +16: 0+109e0 +0 SECTION LOCAL +DEFAULT +16 
 +17: 0+10a10 +0 SECTION LOCAL +DEFAULT +17 
 +18: 0+10830 +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
 +19: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND gd
 +20: 0+60 +0 TLS +GLOBAL DEFAULT +10 le0
 +21: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND __tls_get_addr
 +22: 0+40 +0 TLS +GLOBAL DEFAULT +10 ld0
 +23: 0+68 +0 TLS +GLOBAL DEFAULT +10 le1
 +24: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND ld
 +25: 0+734 +0 NOTYPE +GLOBAL DEFAULT +6 _start
 +26: 0+10a10 +0 NOTYPE +GLOBAL DEFAULT +ABS __end
 +27: 0+50 +0 TLS +GLOBAL DEFAULT +10 ld2
 +28: 0+48 +0 TLS +GLOBAL DEFAULT +10 ld1
 +29: 0+109e0 +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
 +30: 0+109e0 +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
 +31: 0+10a10 +0 NOTYPE +GLOBAL DEFAULT +ABS _end
 +32: 0+38 +0 TLS +GLOBAL DEFAULT +10 gd0
 +33: 0+58 +0 TLS +GLOBAL DEFAULT +10 ie0

Symbol table '\.symtab' contains 45 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +0: 0+ +0 NOTYPE +LOCAL +DEFAULT +UND 
 +1: 0+120 +0 SECTION LOCAL +DEFAULT +1 
 +2: 0+1f8 +0 SECTION LOCAL +DEFAULT +2 
 +3: 0+528 +0 SECTION LOCAL +DEFAULT +3 
 +4: 0+580 +0 SECTION LOCAL +DEFAULT +4 
 +5: 0+700 +0 SECTION LOCAL +DEFAULT +5 
 +6: 0+718 +0 SECTION LOCAL +DEFAULT +6 
 +7: 0+107f8 +0 SECTION LOCAL +DEFAULT +7 
 +8: 0+107f8 +0 SECTION LOCAL +DEFAULT +8 
 +9: 0+107f8 +0 SECTION LOCAL +DEFAULT +9 
 +10: 0+10830 +0 SECTION LOCAL +DEFAULT +10 
 +11: 0+10830 +0 SECTION LOCAL +DEFAULT +11 
 +12: 0+10980 +0 SECTION LOCAL +DEFAULT +12 
 +13: 0+10980 +0 SECTION LOCAL +DEFAULT +13 
 +14: 0+10980 +0 SECTION LOCAL +DEFAULT +14 
 +15: 0+109e0 +0 SECTION LOCAL +DEFAULT +15 
 +16: 0+109e0 +0 SECTION LOCAL +DEFAULT +16 
 +17: 0+10a10 +0 SECTION LOCAL +DEFAULT +17 
 +18: 0+ +0 SECTION LOCAL +DEFAULT +18 
 +19: 0+ +0 SECTION LOCAL +DEFAULT +19 
 +20: 0+ +0 SECTION LOCAL +DEFAULT +20 
 +21: 0+ +0 TLS +LOCAL +DEFAULT +9 gd4
 +22: 0+8 +0 TLS +LOCAL +DEFAULT +9 ld4
 +23: 0+10 +0 TLS +LOCAL +DEFAULT +9 ld5
 +24: 0+18 +0 TLS +LOCAL +DEFAULT +9 ld6
 +25: 0+20 +0 TLS +LOCAL +DEFAULT +9 ie4
 +26: 0+28 +0 TLS +LOCAL +DEFAULT +9 le4
 +27: 0+30 +0 TLS +LOCAL +DEFAULT +9 le5
 +28: 0+718 +0 NOTYPE +LOCAL +DEFAULT +6 \.__tls_get_addr
 +29: 0+10830 +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
 +30: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND gd
 +31: 0+60 +0 TLS +GLOBAL DEFAULT +10 le0
 +32: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND __tls_get_addr
 +33: 0+40 +0 TLS +GLOBAL DEFAULT +10 ld0
 +34: 0+68 +0 TLS +GLOBAL DEFAULT +10 le1
 +35: 0+ +0 NOTYPE +GLOBAL DEFAULT +UND ld
 +36: 0+734 +0 NOTYPE +GLOBAL DEFAULT +6 _start
 +37: 0+10a10 +0 NOTYPE +GLOBAL DEFAULT +ABS __end
 +38: 0+50 +0 TLS +GLOBAL DEFAULT +10 ld2
 +39: 0+48 +0 TLS +GLOBAL DEFAULT +10 ld1
 +40: 0+109e0 +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
 +41: 0+109e0 +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
 +42: 0+10a10 +0 NOTYPE +GLOBAL DEFAULT +ABS _end
 +43: 0+38 +0 TLS +GLOBAL DEFAULT +10 gd0
 +44: 0+58 +0 TLS +GLOBAL DEFAULT +10 ie0
