.*

Section Headers:
 +\[Nr\] Name +Type +Addr +Off +Size +ES Flg Lk Inf Al
 +\[ 0\] +NULL +00000000 000000 000000 00 +0 +0 +0
 +\[ 1\] \.init +PROGBITS +00001000 000080 000004 00 AXp +0 +0 +4
 +\[ 2\] \.text +PROGBITS +00001004 000084 0000d8 00 AXp +0 +0 +4
 +\[ 3\] \.stack +PROGBITS +00080000 000180 000004 00 +WA +0 +0 +1
 +\[ 4\] \.cranges +LOUSER\+1 +00000000 000184 00003c 00 +W +0 +0 +1
 +\[ 5\] \.shstrtab +STRTAB +.*
 +\[ 6\] \.symtab +SYMTAB +.*
 +\[ 7\] \.strtab +STRTAB +.*
Key to Flags:
#...

Symbol table '\.symtab' contains [0-9]+ entries:
 +Num: +Value +Size +Type +Bind +Vis +Ndx +Name
.*: 00000000 +0 +NOTYPE +LOCAL +DEFAULT +UND 
.*: 00001000 +0 +SECTION +LOCAL +DEFAULT +1 
.*: 00001004 +0 +SECTION +LOCAL +DEFAULT +2 
.*: 00080000 +0 +SECTION +LOCAL +DEFAULT +3 
.*: 00000000 +0 +SECTION +LOCAL +DEFAULT +4 
.*: 00000000 +0 +FILE +LOCAL +DEFAULT +ABS .*
.*: 00001004 +0 +NOTYPE +LOCAL +DEFAULT +2 sec4
.*: 00000000 +0 +FILE +LOCAL +DEFAULT +ABS .*
.*: 000010a4 +0 +NOTYPE +LOCAL +DEFAULT +\[<other>: 4\] +2 start2
.*: 00000000 +0 +FILE +LOCAL +DEFAULT +ABS .*
.*: 000010bc +0 +NOTYPE +LOCAL +DEFAULT +2 sec3
.*: 000010c4 +0 +NOTYPE +GLOBAL +DEFAULT +\[<other>: 4\] +2 diversion
.*: 00001160 +0 +NOTYPE +GLOBAL +DEFAULT +.* ___dtors
.*: 00001160 +0 +NOTYPE +GLOBAL +DEFAULT +2 __bss_start
.*: 00001160 +0 +NOTYPE +GLOBAL +DEFAULT +.* ___ctors_end
.*: 000010a4 +0 +NOTYPE +GLOBAL +DEFAULT +2 diversion2
.*: 00001160 +0 +NOTYPE +GLOBAL +DEFAULT +.* ___ctors
.*: 00001160 +0 +NOTYPE +GLOBAL +DEFAULT +2 _edata
.*: 00001160 +0 +NOTYPE +GLOBAL +DEFAULT +2 _end
.*: 00001000 +0 +NOTYPE +GLOBAL +DEFAULT +\[<other>: 4\] +1 start
.*: 00080000 +0 +NOTYPE +GLOBAL +DEFAULT +3 _stack
.*: 00001160 +0 +NOTYPE +GLOBAL +DEFAULT +.* ___dtors_end

Hex dump of section '\.text':
  0x00001004 6ff0fff0 6ff0fff0 6ff0fff0 6ff0fff0 .*
  0x00001014 6ff0fff0 6ff0fff0 cc00bd40 6ff0fff0 .*
  0x00001024 6ff0fff0 6ff0fff0 6ff0fff0 6ff0fff0 .*
  0x00001034 6ff0fff0 6ff0fff0 6ff0fff0 6ff0fff0 .*
  0x00001044 6ff0fff0 6ff0fff0 6ff0fff0 cc00bd50 .*
  0x00001054 cc0084c0 6ff0fff0 6ff0fff0 6ff0fff0 .*
  0x00001064 6ff0fff0 6ff0fff0 6ff0fff0 6ff0fff0 .*
  0x00001074 6ff0fff0 6ff0fff0 6ff0fff0 6ff0fff0 .*
  0x00001084 6ff0fff0 6ff0fff0 6ff0fff0 6ff0fff0 .*
  0x00001094 6ff0fff0 6ff0fff0 6ff0fff0 cc0084d0 .*
  0x000010a4 6ff0fff0 6ff0fff0 6ff0fff0 0000002a .*
  0x000010b4 0000002b 00090009 00090009 00090000 .*
  0x000010c4 e10f0009 00090009 00090009 00090009 .*
  0x000010d4 00090009 0009e10e                   .*

Hex dump of section '\.cranges':
  0x00000000 00001004 000000a0 00030000 10a40000 .*
  0x00000010 000c0003 000010b0 00000008 00010000 .*
  0x00000020 10b80000 00040002 000010bc 00000006 .*
  0x00000030 00020000 10c40000 00180002          .*
