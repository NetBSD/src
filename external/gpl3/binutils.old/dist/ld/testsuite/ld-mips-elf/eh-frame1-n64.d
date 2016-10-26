#name: MIPS eh-frame 1, n64
#source: eh-frame1.s
#source: eh-frame1.s
#as: -march=from-abi -EB -64 --defsym alignment=3 --defsym fill=0x40
#readelf: --relocs -wf
#ld: -shared -melf64btsmip -Teh-frame1.ld
#warning: FDE encoding in.*prevents \.eh_frame_hdr table being created.

Relocation section '\.rel\.dyn' .*:
 *Offset .*
0+00+000  [0-9a-f]+ R_MIPS_NONE *
 *Type2: R_MIPS_NONE *
 *Type3: R_MIPS_NONE *
# Initial PCs for the FDEs attached to CIE 0x120
0+00030148  [0-9a-f]+ R_MIPS_REL32 *
 *Type2: R_MIPS_64 *
 *Type3: R_MIPS_NONE *
0+00030168  [0-9a-f]+ R_MIPS_REL32 *
 *Type2: R_MIPS_64 *
 *Type3: R_MIPS_NONE *
0+00030308  [0-9a-f]+ R_MIPS_REL32 *
 *Type2: R_MIPS_64 *
 *Type3: R_MIPS_NONE *
0+00030328  [0-9a-f]+ R_MIPS_REL32 *
 *Type2: R_MIPS_64 *
 *Type3: R_MIPS_NONE *
0+000300cb  [0-9a-f]+ R_MIPS_REL32      0+00+00+00 foo
 *Type2: R_MIPS_64 *
 *Type3: R_MIPS_NONE *
0+00030138  [0-9a-f]+ R_MIPS_REL32      0+00+00+00 foo
 *Type2: R_MIPS_64 *
 *Type3: R_MIPS_NONE *
0+00030192  [0-9a-f]+ R_MIPS_REL32      0+00+00+00 foo
 *Type2: R_MIPS_64 *
 *Type3: R_MIPS_NONE *
Contents of the \.eh_frame section:

0+0000 0+0014 0+0000 CIE
  Version:               1
  Augmentation:          "zR"
  Code alignment factor: 1
  Data alignment factor: 4
  Return address column: 31
  Augmentation data:     1c

  DW_CFA_advance_loc: 0 to 0+0000
  DW_CFA_advance_loc: 0 to 0+0000
  DW_CFA_advance_loc: 0 to 0+0000
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+0018 0+001c 0+001c FDE cie=0+0000 pc=0+020000..0+020010
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+0038 0+001c 0+003c FDE cie=0+0000 pc=0+020010..0+020030
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

# basic2 removed
0+0058 0+001c 0+005c FDE cie=0+0000 pc=0+020030..0+020060
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

# basic3 removed
0+0078 0+001c 0+007c FDE cie=0+0000 pc=0+020060..0+0200a0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

# basic4 removed
0+0098 0+001c 0+009c FDE cie=0+0000 pc=0+0200a0..0+0200f0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+00b8 0+0024 0+0000 CIE
  Version:               1
  Augmentation:          "zRP"
  Code alignment factor: 1
  Data alignment factor: 4
  Return address column: 31
  Augmentation data:     1c 00 00 00 00 00 00 00 00 00

  DW_CFA_advance_loc: 0 to 0+0000
  DW_CFA_advance_loc: 0 to 0+0000
  DW_CFA_advance_loc: 0 to 0+0000
  DW_CFA_advance_loc: 0 to 0+0000
  DW_CFA_advance_loc: 0 to 0+0000
  DW_CFA_advance_loc: 0 to 0+0000
  DW_CFA_advance_loc: 0 to 0+0000
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+00e0 0+001c 0+002c FDE cie=0+00b8 pc=0+0200f0..0+020100
  DW_CFA_advance_loc: 0 to 0+0200f0
  DW_CFA_advance_loc: 0 to 0+0200f0
  DW_CFA_advance_loc: 0 to 0+0200f0
  DW_CFA_advance_loc: 0 to 0+0200f0
  DW_CFA_advance_loc: 0 to 0+0200f0
  DW_CFA_advance_loc: 0 to 0+0200f0
  DW_CFA_advance_loc: 0 to 0+0200f0

0+0100 0+001c 0+004c FDE cie=0+00b8 pc=0+020100..0+020120
  DW_CFA_advance_loc: 0 to 0+020100
  DW_CFA_advance_loc: 0 to 0+020100
  DW_CFA_advance_loc: 0 to 0+020100
  DW_CFA_advance_loc: 0 to 0+020100
  DW_CFA_advance_loc: 0 to 0+020100
  DW_CFA_advance_loc: 0 to 0+020100
  DW_CFA_advance_loc: 0 to 0+020100

0+0120 0+001c 0+0000 CIE
  Version:               1
  Augmentation:          "zP"
  Code alignment factor: 1
  Data alignment factor: 4
  Return address column: 31
  Augmentation data:     50 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00


0+0140 0+001c 0+0024 FDE cie=0+0120 pc=0+020120..0+020130
  DW_CFA_advance_loc: 0 to 0+020120
  DW_CFA_advance_loc: 0 to 0+020120
  DW_CFA_advance_loc: 0 to 0+020120
  DW_CFA_advance_loc: 0 to 0+020120
  DW_CFA_advance_loc: 0 to 0+020120
  DW_CFA_advance_loc: 0 to 0+020120
  DW_CFA_advance_loc: 0 to 0+020120

0+0160 0+001c 0+0044 FDE cie=0+0120 pc=0+020130..0+020150
  DW_CFA_advance_loc: 0 to 0+020130
  DW_CFA_advance_loc: 0 to 0+020130
  DW_CFA_advance_loc: 0 to 0+020130
  DW_CFA_advance_loc: 0 to 0+020130
  DW_CFA_advance_loc: 0 to 0+020130
  DW_CFA_advance_loc: 0 to 0+020130
  DW_CFA_advance_loc: 0 to 0+020130

0+0180 0+001c 0+0000 CIE
  Version:               1
  Augmentation:          "zPR"
  Code alignment factor: 1
  Data alignment factor: 4
  Return address column: 31
  Augmentation data:     00 00 00 00 00 00 00 00 00 1c

  DW_CFA_advance_loc: 0 to 0+0000
  DW_CFA_advance_loc: 0 to 0+0000
  DW_CFA_advance_loc: 0 to 0+0000
  DW_CFA_advance_loc: 0 to 0+0000
  DW_CFA_advance_loc: 0 to 0+0000

0+01a0 0+001c 0+0024 FDE cie=0+0180 pc=0+020150..0+020160
  DW_CFA_advance_loc: 0 to 0+020150
  DW_CFA_advance_loc: 0 to 0+020150
  DW_CFA_advance_loc: 0 to 0+020150
  DW_CFA_advance_loc: 0 to 0+020150
  DW_CFA_advance_loc: 0 to 0+020150
  DW_CFA_advance_loc: 0 to 0+020150
  DW_CFA_advance_loc: 0 to 0+020150

# FDE for .discard removed
# zPR2 removed
0+01c0 0+001c 0+0044 FDE cie=0+0180 pc=0+020160..0+020190
  DW_CFA_advance_loc: 0 to 0+020160
  DW_CFA_advance_loc: 0 to 0+020160
  DW_CFA_advance_loc: 0 to 0+020160
  DW_CFA_advance_loc: 0 to 0+020160
  DW_CFA_advance_loc: 0 to 0+020160
  DW_CFA_advance_loc: 0 to 0+020160
  DW_CFA_advance_loc: 0 to 0+020160

0+01e0 0+001c 0+0064 FDE cie=0+0180 pc=0+020190..0+0201d0
  DW_CFA_advance_loc: 0 to 0+020190
  DW_CFA_advance_loc: 0 to 0+020190
  DW_CFA_advance_loc: 0 to 0+020190
  DW_CFA_advance_loc: 0 to 0+020190
  DW_CFA_advance_loc: 0 to 0+020190
  DW_CFA_advance_loc: 0 to 0+020190
  DW_CFA_advance_loc: 0 to 0+020190

0+0200 0+001c 0+0204 FDE cie=0+0000 pc=0+0201d0..0+0201e0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

# basic1 removed, followed by repeat of above
0+0220 0+001c 0+0224 FDE cie=0+0000 pc=0+0201e0..0+0201f0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+0240 0+001c 0+0244 FDE cie=0+0000 pc=0+0201f0..0+020210
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+0260 0+001c 0+0264 FDE cie=0+0000 pc=0+020210..0+020240
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+0280 0+001c 0+0284 FDE cie=0+0000 pc=0+020240..0+020280
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+02a0 0+001c 0+02a4 FDE cie=0+0000 pc=0+020280..0+0202d0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+02c0 0+001c 0+020c FDE cie=0+00b8 pc=0+0202d0..0+0202e0
  DW_CFA_advance_loc: 0 to 0+0202d0
  DW_CFA_advance_loc: 0 to 0+0202d0
  DW_CFA_advance_loc: 0 to 0+0202d0
  DW_CFA_advance_loc: 0 to 0+0202d0
  DW_CFA_advance_loc: 0 to 0+0202d0
  DW_CFA_advance_loc: 0 to 0+0202d0
  DW_CFA_advance_loc: 0 to 0+0202d0

0+02e0 0+001c 0+022c FDE cie=0+00b8 pc=0+0202e0..0+020300
  DW_CFA_advance_loc: 0 to 0+0202e0
  DW_CFA_advance_loc: 0 to 0+0202e0
  DW_CFA_advance_loc: 0 to 0+0202e0
  DW_CFA_advance_loc: 0 to 0+0202e0
  DW_CFA_advance_loc: 0 to 0+0202e0
  DW_CFA_advance_loc: 0 to 0+0202e0
  DW_CFA_advance_loc: 0 to 0+0202e0

0+0300 0+001c 0+01e4 FDE cie=0+0120 pc=0+020300..0+020310
  DW_CFA_advance_loc: 0 to 0+020300
  DW_CFA_advance_loc: 0 to 0+020300
  DW_CFA_advance_loc: 0 to 0+020300
  DW_CFA_advance_loc: 0 to 0+020300
  DW_CFA_advance_loc: 0 to 0+020300
  DW_CFA_advance_loc: 0 to 0+020300
  DW_CFA_advance_loc: 0 to 0+020300

0+0320 0+001c 0+0204 FDE cie=0+0120 pc=0+020310..0+020330
  DW_CFA_advance_loc: 0 to 0+020310
  DW_CFA_advance_loc: 0 to 0+020310
  DW_CFA_advance_loc: 0 to 0+020310
  DW_CFA_advance_loc: 0 to 0+020310
  DW_CFA_advance_loc: 0 to 0+020310
  DW_CFA_advance_loc: 0 to 0+020310
  DW_CFA_advance_loc: 0 to 0+020310

0+0340 0+001c 0+01c4 FDE cie=0+0180 pc=0+020330..0+020340
  DW_CFA_advance_loc: 0 to 0+020330
  DW_CFA_advance_loc: 0 to 0+020330
  DW_CFA_advance_loc: 0 to 0+020330
  DW_CFA_advance_loc: 0 to 0+020330
  DW_CFA_advance_loc: 0 to 0+020330
  DW_CFA_advance_loc: 0 to 0+020330
  DW_CFA_advance_loc: 0 to 0+020330

0+0360 0+001c 0+01e4 FDE cie=0+0180 pc=0+020340..0+020370
  DW_CFA_advance_loc: 0 to 0+020340
  DW_CFA_advance_loc: 0 to 0+020340
  DW_CFA_advance_loc: 0 to 0+020340
  DW_CFA_advance_loc: 0 to 0+020340
  DW_CFA_advance_loc: 0 to 0+020340
  DW_CFA_advance_loc: 0 to 0+020340
  DW_CFA_advance_loc: 0 to 0+020340

0+0380 0+001c 0+0204 FDE cie=0+0180 pc=0+020370..0+0203b0
  DW_CFA_advance_loc: 0 to 0+020370
  DW_CFA_advance_loc: 0 to 0+020370
  DW_CFA_advance_loc: 0 to 0+020370
  DW_CFA_advance_loc: 0 to 0+020370
  DW_CFA_advance_loc: 0 to 0+020370
  DW_CFA_advance_loc: 0 to 0+020370
  DW_CFA_advance_loc: 0 to 0+020370

0+03a0 0+001c 0+03a4 FDE cie=0+0000 pc=0+0203b0..0+0203c0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

