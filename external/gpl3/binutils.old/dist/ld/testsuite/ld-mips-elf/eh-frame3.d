#name: MIPS eh-frame 3
#source: eh-frame1.s
#source: eh-frame1.s
#as: -EB -mips3 -mabi=eabi --defsym alignment=3 --defsym fill=0
#readelf: -wf
#ld: -EB -Teh-frame1.ld --defsym foo=0x50607080
#
# This test is for the official LP64 version of EABI64, which uses a
# combination of 32-bit objects and 64-bit FDE addresses.
#

Contents of the \.eh_frame section:

00000000 0000000c 00000000 CIE
  Version:               1
  Augmentation:          ""
  Code alignment factor: 1
  Data alignment factor: 4
  Return address column: 31

  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000010 00000014 00000014 FDE cie=00000000 pc=00020000..00020010

00000028 00000014 0000002c FDE cie=00000000 pc=00020010..00020030

# basic2 removed
00000040 00000014 00000044 FDE cie=00000000 pc=00020030..00020060

# basic3 removed
00000058 00000014 0000005c FDE cie=00000000 pc=00020060..000200a0

# basic4 removed
00000070 00000014 00000074 FDE cie=00000000 pc=000200a0..000200f0

00000088 0000001c 00000000 CIE
  Version:               1
  Augmentation:          "zP"
  Code alignment factor: 1
  Data alignment factor: 4
  Return address column: 31
  Augmentation data:     00 00 00 00 00 50 60 70 80

  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

000000a8 0000001c 00000024 FDE cie=00000088 pc=000200f0..00020100
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

000000c8 0000001c 00000044 FDE cie=00000088 pc=00020100..00020120
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

000000e8 0000001c 00000000 CIE
  Version:               1
  Augmentation:          "zP"
  Code alignment factor: 1
  Data alignment factor: 4
  Return address column: 31
  Augmentation data:     50 00 00 00 00 00 00 00 00 00 00 00 50 60 70 80


00000108 0000001c 00000024 FDE cie=000000e8 pc=00020120..00020130
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000128 0000001c 00000044 FDE cie=000000e8 pc=00020130..00020150
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000148 0000001c 00000000 CIE
  Version:               1
  Augmentation:          "zPR"
  Code alignment factor: 1
  Data alignment factor: 4
  Return address column: 31
  Augmentation data:     00 00 00 00 00 50 60 70 80 00

  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000168 0000001c 00000024 FDE cie=00000148 pc=00020150..00020160
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

# FDE for .discard removed
# zPR2 removed
00000188 0000001c 00000044 FDE cie=00000148 pc=00020160..00020190
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

000001a8 0000001c 00000064 FDE cie=00000148 pc=00020190..000201d0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

000001c8 00000014 000001cc FDE cie=00000000 pc=000201d0..000201e0

# basic1 removed, followed by repeat of above
000001e0 00000014 000001e4 FDE cie=00000000 pc=000201e0..000201f0

000001f8 00000014 000001fc FDE cie=00000000 pc=000201f0..00020210

00000210 00000014 00000214 FDE cie=00000000 pc=00020210..00020240

00000228 00000014 0000022c FDE cie=00000000 pc=00020240..00020280

00000240 00000014 00000244 FDE cie=00000000 pc=00020280..000202d0

00000258 0000001c 000001d4 FDE cie=00000088 pc=000202d0..000202e0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000278 0000001c 000001f4 FDE cie=00000088 pc=000202e0..00020300
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000298 0000001c 000001b4 FDE cie=000000e8 pc=00020300..00020310
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

000002b8 0000001c 000001d4 FDE cie=000000e8 pc=00020310..00020330
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

000002d8 0000001c 00000194 FDE cie=00000148 pc=00020330..00020340
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

# FDE for .discard removed
# zPR2 removed
000002f8 0000001c 000001b4 FDE cie=00000148 pc=00020340..00020370
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000318 0000001c 000001d4 FDE cie=00000148 pc=00020370..000203b0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000338 00000014 0000033c FDE cie=00000000 pc=000203b0..000203c0
