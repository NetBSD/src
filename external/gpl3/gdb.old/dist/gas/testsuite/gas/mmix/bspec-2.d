#readelf: -Sr -T -x1 -x4
There are 11 section headers, starting at offset .*:
#...
  \[ 4\] \.MMIX\.spec_data\.2 PROGBITS         0+  0+48
       0+10  0+           0     0     8
  \[ 5\] \.rela\.MMIX\.spec_d RELA             0+  .*
 +0+30  0+18 +I +8 +4 +8
  \[ 6\] \.MMIX\.spec_data\.3 PROGBITS         0+  0+58
       0+8  0+           0     0     8
  \[ 7\] \.rela\.MMIX\.spec_d RELA             0+  .*
 +0+18 +0+18 +I +8 +6 +8
#...
Relocation section '\.rela\.MMIX\.spec_data\.2' at offset .* contains 2 entries:
.*
0+  0+600000004 R_MMIX_32 +0+ +forw +\+ 0
0+8  0+700000005 R_MMIX_64 +0+ +other +\+ 0

Relocation section '\.rela\.MMIX\.spec_data\.3' at offset .* contains 1 entry:
.*
0+  0+700000005 R_MMIX_64 +0+ +other +\+ 0

Hex dump of section '\.text':
  0x0+ fd010203                            .*

Hex dump of section '\.MMIX\.spec_data\.2':
 NOTE: This section has relocations against it, but these have NOT been applied to this dump.
  0x0+ 00000000 0000002a 00000000 00000000 .*
