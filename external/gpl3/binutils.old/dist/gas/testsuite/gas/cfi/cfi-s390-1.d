#readelf: -wf
#name: CFI on s390
#as: -m31 -march=g5

Contents of the .eh_frame section:

0+0000 0+0010 0+0000 CIE
  Version:               1
  Augmentation:          "zR"
  Code alignment factor: 1
  Data alignment factor: -4
  Return address column: 14
  Augmentation data:     1b

  DW_CFA_def_cfa: r15 ofs 96

0+0014 0+0024 0+0018 FDE cie=0+0000 pc=0+0000..0+004e
  DW_CFA_advance_loc: 4 to 0+0004
  DW_CFA_offset: r15 at cfa-36
  DW_CFA_offset: r14 at cfa-40
  DW_CFA_offset: r13 at cfa-44
  DW_CFA_offset: r12 at cfa-48
  DW_CFA_offset: r11 at cfa-52
  DW_CFA_offset: r10 at cfa-56
  DW_CFA_offset: r9 at cfa-60
  DW_CFA_offset: r8 at cfa-64
  DW_CFA_advance_loc: 22 to 0+001a
  DW_CFA_def_cfa_offset: 192
  DW_CFA_nop
  DW_CFA_nop

