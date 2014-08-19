#name: MIPS eh-frame 1, n64
#source: eh-frame1.s
#source: eh-frame1.s
#as: -march=from-abi -EB -64 --defsym alignment=3 --defsym fill=0x40
#readelf: --relocs -wf
#ld: -shared -melf64btsmip -Teh-frame1.ld
#warning: fde encoding in.*prevents \.eh_frame_hdr table being created.

Relocation section '\.rel\.dyn' .*:
 *Offset .*
000000000000  [0-9a-f]+ R_MIPS_NONE *
 *Type2: R_MIPS_NONE *
 *Type3: R_MIPS_NONE *
# Initial PCs for the FDEs attached to CIE 0x120
000000030148  [0-9a-f]+ R_MIPS_REL32 *
 *Type2: R_MIPS_64 *
 *Type3: R_MIPS_NONE *
000000030168  [0-9a-f]+ R_MIPS_REL32 *
 *Type2: R_MIPS_64 *
 *Type3: R_MIPS_NONE *
000000030308  [0-9a-f]+ R_MIPS_REL32 *
 *Type2: R_MIPS_64 *
 *Type3: R_MIPS_NONE *
000000030328  [0-9a-f]+ R_MIPS_REL32 *
 *Type2: R_MIPS_64 *
 *Type3: R_MIPS_NONE *
0000000300cb  [0-9a-f]+ R_MIPS_REL32      0000000000000000 foo
 *Type2: R_MIPS_64 *
 *Type3: R_MIPS_NONE *
000000030138  [0-9a-f]+ R_MIPS_REL32      0000000000000000 foo
 *Type2: R_MIPS_64 *
 *Type3: R_MIPS_NONE *
000000030192  [0-9a-f]+ R_MIPS_REL32      0000000000000000 foo
 *Type2: R_MIPS_64 *
 *Type3: R_MIPS_NONE *
Contents of the \.eh_frame section:

00000000 00000014 00000000 CIE
  Version:               1
  Augmentation:          "zR"
  Code alignment factor: 1
  Data alignment factor: 4
  Return address column: 31
  Augmentation data:     1c

  DW_CFA_advance_loc: 0 to 00000000
  DW_CFA_advance_loc: 0 to 00000000
  DW_CFA_advance_loc: 0 to 00000000
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000018 0000001c 0000001c FDE cie=00000000 pc=00020000..00020010
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000038 0000001c 0000003c FDE cie=00000000 pc=00020010..00020030
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

# basic2 removed
00000058 0000001c 0000005c FDE cie=00000000 pc=00020030..00020060
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

# basic3 removed
00000078 0000001c 0000007c FDE cie=00000000 pc=00020060..000200a0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

# basic4 removed
00000098 0000001c 0000009c FDE cie=00000000 pc=000200a0..000200f0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

000000b8 00000024 00000000 CIE
  Version:               1
  Augmentation:          "zRP"
  Code alignment factor: 1
  Data alignment factor: 4
  Return address column: 31
  Augmentation data:     1c 00 00 00 00 00 00 00 00 00

  DW_CFA_advance_loc: 0 to 00000000
  DW_CFA_advance_loc: 0 to 00000000
  DW_CFA_advance_loc: 0 to 00000000
  DW_CFA_advance_loc: 0 to 00000000
  DW_CFA_advance_loc: 0 to 00000000
  DW_CFA_advance_loc: 0 to 00000000
  DW_CFA_advance_loc: 0 to 00000000
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

000000e0 0000001c 0000002c FDE cie=000000b8 pc=000200f0..00020100
  DW_CFA_advance_loc: 0 to 000200f0
  DW_CFA_advance_loc: 0 to 000200f0
  DW_CFA_advance_loc: 0 to 000200f0
  DW_CFA_advance_loc: 0 to 000200f0
  DW_CFA_advance_loc: 0 to 000200f0
  DW_CFA_advance_loc: 0 to 000200f0
  DW_CFA_advance_loc: 0 to 000200f0

00000100 0000001c 0000004c FDE cie=000000b8 pc=00020100..00020120
  DW_CFA_advance_loc: 0 to 00020100
  DW_CFA_advance_loc: 0 to 00020100
  DW_CFA_advance_loc: 0 to 00020100
  DW_CFA_advance_loc: 0 to 00020100
  DW_CFA_advance_loc: 0 to 00020100
  DW_CFA_advance_loc: 0 to 00020100
  DW_CFA_advance_loc: 0 to 00020100

00000120 0000001c 00000000 CIE
  Version:               1
  Augmentation:          "zP"
  Code alignment factor: 1
  Data alignment factor: 4
  Return address column: 31
  Augmentation data:     50 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00


00000140 0000001c 00000024 FDE cie=00000120 pc=00020120..00020130
  DW_CFA_advance_loc: 0 to 00020120
  DW_CFA_advance_loc: 0 to 00020120
  DW_CFA_advance_loc: 0 to 00020120
  DW_CFA_advance_loc: 0 to 00020120
  DW_CFA_advance_loc: 0 to 00020120
  DW_CFA_advance_loc: 0 to 00020120
  DW_CFA_advance_loc: 0 to 00020120

00000160 0000001c 00000044 FDE cie=00000120 pc=00020130..00020150
  DW_CFA_advance_loc: 0 to 00020130
  DW_CFA_advance_loc: 0 to 00020130
  DW_CFA_advance_loc: 0 to 00020130
  DW_CFA_advance_loc: 0 to 00020130
  DW_CFA_advance_loc: 0 to 00020130
  DW_CFA_advance_loc: 0 to 00020130
  DW_CFA_advance_loc: 0 to 00020130

00000180 0000001c 00000000 CIE
  Version:               1
  Augmentation:          "zPR"
  Code alignment factor: 1
  Data alignment factor: 4
  Return address column: 31
  Augmentation data:     00 00 00 00 00 00 00 00 00 1c

  DW_CFA_advance_loc: 0 to 00000000
  DW_CFA_advance_loc: 0 to 00000000
  DW_CFA_advance_loc: 0 to 00000000
  DW_CFA_advance_loc: 0 to 00000000
  DW_CFA_advance_loc: 0 to 00000000

000001a0 0000001c 00000024 FDE cie=00000180 pc=00020150..00020160
  DW_CFA_advance_loc: 0 to 00020150
  DW_CFA_advance_loc: 0 to 00020150
  DW_CFA_advance_loc: 0 to 00020150
  DW_CFA_advance_loc: 0 to 00020150
  DW_CFA_advance_loc: 0 to 00020150
  DW_CFA_advance_loc: 0 to 00020150
  DW_CFA_advance_loc: 0 to 00020150

# FDE for .discard removed
# zPR2 removed
000001c0 0000001c 00000044 FDE cie=00000180 pc=00020160..00020190
  DW_CFA_advance_loc: 0 to 00020160
  DW_CFA_advance_loc: 0 to 00020160
  DW_CFA_advance_loc: 0 to 00020160
  DW_CFA_advance_loc: 0 to 00020160
  DW_CFA_advance_loc: 0 to 00020160
  DW_CFA_advance_loc: 0 to 00020160
  DW_CFA_advance_loc: 0 to 00020160

000001e0 0000001c 00000064 FDE cie=00000180 pc=00020190..000201d0
  DW_CFA_advance_loc: 0 to 00020190
  DW_CFA_advance_loc: 0 to 00020190
  DW_CFA_advance_loc: 0 to 00020190
  DW_CFA_advance_loc: 0 to 00020190
  DW_CFA_advance_loc: 0 to 00020190
  DW_CFA_advance_loc: 0 to 00020190
  DW_CFA_advance_loc: 0 to 00020190

00000200 0000001c 00000204 FDE cie=00000000 pc=000201d0..000201e0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

# basic1 removed, followed by repeat of above
00000220 0000001c 00000224 FDE cie=00000000 pc=000201e0..000201f0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000240 0000001c 00000244 FDE cie=00000000 pc=000201f0..00020210
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000260 0000001c 00000264 FDE cie=00000000 pc=00020210..00020240
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000280 0000001c 00000284 FDE cie=00000000 pc=00020240..00020280
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

000002a0 0000001c 000002a4 FDE cie=00000000 pc=00020280..000202d0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

000002c0 0000001c 0000020c FDE cie=000000b8 pc=000202d0..000202e0
  DW_CFA_advance_loc: 0 to 000202d0
  DW_CFA_advance_loc: 0 to 000202d0
  DW_CFA_advance_loc: 0 to 000202d0
  DW_CFA_advance_loc: 0 to 000202d0
  DW_CFA_advance_loc: 0 to 000202d0
  DW_CFA_advance_loc: 0 to 000202d0
  DW_CFA_advance_loc: 0 to 000202d0

000002e0 0000001c 0000022c FDE cie=000000b8 pc=000202e0..00020300
  DW_CFA_advance_loc: 0 to 000202e0
  DW_CFA_advance_loc: 0 to 000202e0
  DW_CFA_advance_loc: 0 to 000202e0
  DW_CFA_advance_loc: 0 to 000202e0
  DW_CFA_advance_loc: 0 to 000202e0
  DW_CFA_advance_loc: 0 to 000202e0
  DW_CFA_advance_loc: 0 to 000202e0

00000300 0000001c 000001e4 FDE cie=00000120 pc=00020300..00020310
  DW_CFA_advance_loc: 0 to 00020300
  DW_CFA_advance_loc: 0 to 00020300
  DW_CFA_advance_loc: 0 to 00020300
  DW_CFA_advance_loc: 0 to 00020300
  DW_CFA_advance_loc: 0 to 00020300
  DW_CFA_advance_loc: 0 to 00020300
  DW_CFA_advance_loc: 0 to 00020300

00000320 0000001c 00000204 FDE cie=00000120 pc=00020310..00020330
  DW_CFA_advance_loc: 0 to 00020310
  DW_CFA_advance_loc: 0 to 00020310
  DW_CFA_advance_loc: 0 to 00020310
  DW_CFA_advance_loc: 0 to 00020310
  DW_CFA_advance_loc: 0 to 00020310
  DW_CFA_advance_loc: 0 to 00020310
  DW_CFA_advance_loc: 0 to 00020310

00000340 0000001c 000001c4 FDE cie=00000180 pc=00020330..00020340
  DW_CFA_advance_loc: 0 to 00020330
  DW_CFA_advance_loc: 0 to 00020330
  DW_CFA_advance_loc: 0 to 00020330
  DW_CFA_advance_loc: 0 to 00020330
  DW_CFA_advance_loc: 0 to 00020330
  DW_CFA_advance_loc: 0 to 00020330
  DW_CFA_advance_loc: 0 to 00020330

00000360 0000001c 000001e4 FDE cie=00000180 pc=00020340..00020370
  DW_CFA_advance_loc: 0 to 00020340
  DW_CFA_advance_loc: 0 to 00020340
  DW_CFA_advance_loc: 0 to 00020340
  DW_CFA_advance_loc: 0 to 00020340
  DW_CFA_advance_loc: 0 to 00020340
  DW_CFA_advance_loc: 0 to 00020340
  DW_CFA_advance_loc: 0 to 00020340

00000380 0000001c 00000204 FDE cie=00000180 pc=00020370..000203b0
  DW_CFA_advance_loc: 0 to 00020370
  DW_CFA_advance_loc: 0 to 00020370
  DW_CFA_advance_loc: 0 to 00020370
  DW_CFA_advance_loc: 0 to 00020370
  DW_CFA_advance_loc: 0 to 00020370
  DW_CFA_advance_loc: 0 to 00020370
  DW_CFA_advance_loc: 0 to 00020370

000003a0 0000001c 000003a4 FDE cie=00000000 pc=000203b0..000203c0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

