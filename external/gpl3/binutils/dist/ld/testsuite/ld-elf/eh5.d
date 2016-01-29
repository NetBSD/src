#source: eh5.s
#source: eh5a.s
#source: eh5b.s
#ld:
#readelf: -wf
#target: cfi
#notarget: alpha* hppa64* tile* visium*

Contents of the .eh_frame section:

0+0000 0+001[04] 0+0000 CIE
  Version:               1
  Augmentation:          "zR"
  Code alignment factor: .*
  Data alignment factor: .*
  Return address column: .*
  Augmentation data:     (0b|1b)

  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
#...
0+001[48] 0+0014 0+001[8c] FDE cie=0+0000 pc=.*
  DW_CFA_advance_loc: 4 to .*
  DW_CFA_def_cfa: r0(.*) ofs 16
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+00(2c|30) 0+0014 0+0000 CIE
  Version:               1
  Augmentation:          "zPR"
  Code alignment factor: .*
  Data alignment factor: .*
  Return address column: .*
  Augmentation data:     03 .. .. .. .. (0b|1b)

  DW_CFA_nop

0+004[48] 0+0014 0+001c FDE cie=0+00(2c|30) pc=.*
  DW_CFA_advance_loc: 4 to .*
  DW_CFA_def_cfa: r0(.*) ofs 16
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+00(5c|60) 0+0014 0+006[04] FDE cie=0+0000 pc=.*
  DW_CFA_advance_loc: 4 to .*
  DW_CFA_def_cfa: r0(.*) ofs 16
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+007[48] 0+001[8c] 0+0000 CIE
  Version:               1
  Augmentation:          "zPLR"
  Code alignment factor: .*
  Data alignment factor: .*
  Return address column: .*
  Augmentation data:     03 .. .. .. .. 0c (0b|1b)

  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
#...
0+009[08] 0+001c 0+002[04] FDE cie=0+007[48] pc=.*
  Augmentation data:     (ef be ad de 00 00 00 00|00 00 00 00 de ad be ef)

  DW_CFA_advance_loc: 4 to .*
  DW_CFA_def_cfa: r0(.*) ofs 16
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+00b[08] 0+001[04] 0+0000 CIE
  Version:               1
  Augmentation:          "zR"
  Code alignment factor: .*
  Data alignment factor: .*
  Return address column: .*
  Augmentation data:     (0b|1b)

  DW_CFA_def_cfa: r0(.*) ofs 16
#...
0+00(c4|d0) 0+001[04] 0+001[8c] FDE cie=0+00b[08] pc=.*
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
#...
0+00[de]8 0+0014 0+0000 CIE
  Version:               1
  Augmentation:          "zPR"
  Code alignment factor: .*
  Data alignment factor: .*
  Return address column: .*
  Augmentation data:     03 .. .. .. .. (0b|1b)

  DW_CFA_nop

0+0(0f|10)0 0+0014 0+001c FDE cie=0+00[de]8 pc=.*
  DW_CFA_advance_loc: 4 to .*
  DW_CFA_def_cfa: r0(.*) ofs 16
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+01[01]8 0+001[04] 0+00(5c|64) FDE cie=0+00b[08] pc=.*
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
#...
0+01(1c|30) 0+001[8c] 0+0000 CIE
  Version:               1
  Augmentation:          "zPLR"
  Code alignment factor: .*
  Data alignment factor: .*
  Return address column: .*
  Augmentation data:     03 .. .. .. .. 0c (0b|1b)

  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
#...
0+01(38|50) 0+001c 0+002[04] FDE cie=0+01(1c|30) pc=.*
  Augmentation data:     (ef be ad de 00 00 00 00|00 00 00 00 de ad be ef)

  DW_CFA_advance_loc: 4 to .*
  DW_CFA_def_cfa: r0(.*) ofs 16
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+01(58|70) 0+0014 0+01(5c|74) FDE cie=0+0000 pc=.*
  DW_CFA_advance_loc: 4 to .*
  DW_CFA_def_cfa: r0(.*) ofs 16
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
#...
0+01(70|88) 0+0014 0+0(01c|148|15c) FDE cie=0+0(02c|030|170) pc=.*
  DW_CFA_advance_loc: 4 to .*
  DW_CFA_def_cfa: r0(.*) ofs 16
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+01(88|a0) 0+0014 0+01(8c|a4) FDE cie=0+0000 pc=.*
  DW_CFA_advance_loc: 4 to .*
  DW_CFA_def_cfa: r0(.*) ofs 16
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
#...
0+01(a0|b8|d4) 0+001c 0+0(020|130|144) FDE cie=0+0(074|078|1b8) pc=.*
  Augmentation data:     (ef be ad de 00 00 00 00|00 00 00 00 de ad be ef)

  DW_CFA_advance_loc: 4 to .*
  DW_CFA_def_cfa: r0(.*) ofs 16
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

