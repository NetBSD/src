#source: align.s
#source: tlsbinpic.s
#source: tlsbin.s
#as:
#ld: -relax -melf64alpha
#readelf: -WSsrl
#target: alpha*-*-*

There are 19 section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
 +\[Nr\] Name +Type +Address +Off +Size +ES Flg Lk Inf Al
 +\[ 0\] +NULL +0+ 0+ 0+ 0+ +0 +0 +0
 +\[ 1\] .interp +.*
 +\[ 2\] .hash +.*
 +\[ 3\] .dynsym +.*
 +\[ 4\] .dynstr +.*
 +\[ 5\] .rela.dyn +.*
 +\[ 6\] .rela.plt +.*
 +\[ 7\] .text +PROGBITS +0+120001000 0+1000 0+bc 0+ +AX +0 +0 4096
 +\[ 8\] .data +.*
 +\[ 9\] .tdata +PROGBITS +0+120012000 0+2000 0+60 0+ WAT +0 +0 +4
 +\[10\] .tbss +NOBITS +0+120012060 0+2060 0+40 0+ WAT +0 +0 +1
 +\[11\] .dynamic +DYNAMIC +0+120012060 0+2060 0+140 10 +WA +4 +0 +8
 +\[12\] .plt +PROGBITS +0+1200121a0 0+21a0 0+ 0+ WAX +0 +0 +8
 +\[13\] .got +PROGBITS +0+1200121a0 0+21a0 0+10 0+ +WA +0 +0 +8
 +\[14\] .sbss +.*
 +\[15\] .bss +.*
 +\[16\] .shstrtab +.*
 +\[17\] .symtab +.*
 +\[18\] .strtab +.*
#...

Elf file type is EXEC \(Executable file\)
Entry point 0x[0-9a-f]+
There are 6 program headers, starting at offset [0-9]+

Program Headers:
 +Type +Offset +VirtAddr +PhysAddr +FileSiz +MemSiz +Flg Align
 +PHDR +0x0+40 0x0+120000040 0x0+120000040 0x0+150 0x0+150 R E 0x8
 +INTERP +0x0+190 0x0+120000190 0x0+120000190 0x[0-9a-f]+ 0x[0-9a-f]+ R +0x1
.*Requesting program interpreter.*
 +LOAD +0x0+ 0x0+120000000 0x0+120000000 0x0+10bc 0x0+10bc R E 0x10000
 +LOAD +0x0+2000 0x0+120012000 0x0+120012000 0x0+1b0 0x0+1b0 RWE 0x10000
 +DYNAMIC +0x0+2060 0x0+120012060 0x0+120012060 0x0+140 0x0+140 RW +0x8
 +TLS +0x0+2000 0x0+120012000 0x0+120012000 0x0+60 0x0+a0 R +0x4
#...

Relocation section '.rela.dyn' at offset 0x[0-9a-f]+ contains 2 entries:
 +Offset +Info +Type +Symbol's Value +Symbol's Name \+ Addend
0+1200121a0 +0+200000026 R_ALPHA_TPREL64 +0+ sG2 \+ 0
0+1200121a8 +0+600000026 R_ALPHA_TPREL64 +0+ sG1 \+ 0

Symbol table '.dynsym' contains 10 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +0: 0+ +0 NOTYPE +LOCAL +DEFAULT +UND 
 +1: 0+120012060 +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
 +2: 0+ +0 TLS +GLOBAL DEFAULT +UND sG2
 +3: 0+1200121a0 +0 OBJECT +GLOBAL DEFAULT +ABS _PROCEDURE_LINKAGE_TABLE_
 +4: 0+ +4 FUNC +GLOBAL DEFAULT +UND __tls_get_addr
 +5: 0+1200121b0 +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
 +6: 0+ +0 TLS +GLOBAL DEFAULT +UND sG1
 +7: 0+1200121b0 +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
 +8: 0+1200121a0 +0 OBJECT +GLOBAL DEFAULT +ABS _GLOBAL_OFFSET_TABLE_
 +9: 0+1200121b0 +0 NOTYPE +GLOBAL DEFAULT +ABS _end

Symbol table '.symtab' contains 70 entries:
 +Num: +Value +Size Type +Bind +Vis +Ndx Name
 +0: 0+ +0 NOTYPE +LOCAL +DEFAULT +UND 
 +1: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +1 
 +2: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +2 
 +3: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +3 
 +4: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +4 
 +5: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +5 
 +6: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +6 
 +7: 0+120001000 +0 SECTION LOCAL +DEFAULT +7 
 +8: 0+120012000 +0 SECTION LOCAL +DEFAULT +8 
 +9: 0+120012000 +0 SECTION LOCAL +DEFAULT +9 
 +10: 0+120012060 +0 SECTION LOCAL +DEFAULT +10 
 +11: 0+120012060 +0 SECTION LOCAL +DEFAULT +11 
 +12: 0+1200121a0 +0 SECTION LOCAL +DEFAULT +12 
 +13: 0+1200121a0 +0 SECTION LOCAL +DEFAULT +13 
 +14: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +14 
 +15: [0-9a-f]+ +0 SECTION LOCAL +DEFAULT +15 
 +16: 0+ +0 SECTION LOCAL +DEFAULT +16 
 +17: 0+ +0 SECTION LOCAL +DEFAULT +17 
 +18: 0+ +0 SECTION LOCAL +DEFAULT +18 
 +19: 0+20 +0 TLS +LOCAL +DEFAULT +9 sl1
 +20: 0+24 +0 TLS +LOCAL +DEFAULT +9 sl2
 +21: 0+28 +0 TLS +LOCAL +DEFAULT +9 sl3
 +22: 0+2c +0 TLS +LOCAL +DEFAULT +9 sl4
 +23: 0+30 +0 TLS +LOCAL +DEFAULT +9 sl5
 +24: 0+34 +0 TLS +LOCAL +DEFAULT +9 sl6
 +25: 0+38 +0 TLS +LOCAL +DEFAULT +9 sl7
 +26: 0+3c +0 TLS +LOCAL +DEFAULT +9 sl8
 +27: 0+80 +0 TLS +LOCAL +DEFAULT +10 bl1
 +28: 0+84 +0 TLS +LOCAL +DEFAULT +10 bl2
 +29: 0+88 +0 TLS +LOCAL +DEFAULT +10 bl3
 +30: 0+8c +0 TLS +LOCAL +DEFAULT +10 bl4
 +31: 0+90 +0 TLS +LOCAL +DEFAULT +10 bl5
 +32: 0+94 +0 TLS +LOCAL +DEFAULT +10 bl6
 +33: 0+98 +0 TLS +LOCAL +DEFAULT +10 bl7
 +34: 0+9c +0 TLS +LOCAL +DEFAULT +10 bl8
 +35: 0+1c +0 TLS +GLOBAL DEFAULT +9 sg8
 +36: 0+7c +0 TLS +GLOBAL DEFAULT +10 bg8
 +37: 0+74 +0 TLS +GLOBAL DEFAULT +10 bg6
 +38: 0+68 +0 TLS +GLOBAL DEFAULT +10 bg3
 +39: 0+120012060 +0 OBJECT +GLOBAL DEFAULT +ABS _DYNAMIC
 +40: 0+8 +0 TLS +GLOBAL DEFAULT +9 sg3
 +41: 0+48 +0 TLS +GLOBAL HIDDEN +9 sh3
 +42: 0+ +0 TLS +GLOBAL DEFAULT +UND sG2
 +43: 0+c +0 TLS +GLOBAL DEFAULT +9 sg4
 +44: 0+10 +0 TLS +GLOBAL DEFAULT +9 sg5
 +45: 0+1200121a0 +0 OBJECT +GLOBAL DEFAULT +ABS _PROCEDURE_LINKAGE_TABLE_
 +46: 0+70 +0 TLS +GLOBAL DEFAULT +10 bg5
 +47: 0+ +4 FUNC +GLOBAL DEFAULT +UND __tls_get_addr
 +48: 0+58 +0 TLS +GLOBAL HIDDEN +9 sh7
 +49: 0+5c +0 TLS +GLOBAL HIDDEN +9 sh8
 +50: 0+ +0 TLS +GLOBAL DEFAULT +9 sg1
 +51: 0+120001088 +52 FUNC +GLOBAL DEFAULT +7 _start
 +52: 0+4c +0 TLS +GLOBAL HIDDEN +9 sh4
 +53: 0+78 +0 TLS +GLOBAL DEFAULT +10 bg7
 +54: 0+50 +0 TLS +GLOBAL HIDDEN +9 sh5
 +55: 0+1200121b0 +0 NOTYPE +GLOBAL DEFAULT +ABS __bss_start
 +56: 0+120001000 +136 FUNC +GLOBAL DEFAULT +7 fn2
 +57: 0+4 +0 TLS +GLOBAL DEFAULT +9 sg2
 +58: 0+ +0 TLS +GLOBAL DEFAULT +UND sG1
 +59: 0+40 +0 TLS +GLOBAL HIDDEN +9 sh1
 +60: 0+14 +0 TLS +GLOBAL DEFAULT +9 sg6
 +61: 0+18 +0 TLS +GLOBAL DEFAULT +9 sg7
 +62: 0+1200121b0 +0 NOTYPE +GLOBAL DEFAULT +ABS _edata
 +63: 0+1200121a0 +0 OBJECT +GLOBAL DEFAULT +ABS _GLOBAL_OFFSET_TABLE_
 +64: 0+1200121b0 +0 NOTYPE +GLOBAL DEFAULT +ABS _end
 +65: 0+44 +0 TLS +GLOBAL HIDDEN +9 sh2
 +66: 0+54 +0 TLS +GLOBAL HIDDEN +9 sh6
 +67: 0+64 +0 TLS +GLOBAL DEFAULT +10 bg2
 +68: 0+60 +0 TLS +GLOBAL DEFAULT +10 bg1
 +69: 0+6c +0 TLS +GLOBAL DEFAULT +10 bg4
