#readelf: -wf
#name: CFI common 5
Contents of the .eh_frame section:

00000000 0+0010 0+000 CIE
  Version:               1
  Augmentation:          "zR"
  Code alignment factor: .*
  Data alignment factor: .*
  Return address column: .*
  Augmentation data:     [01]b
#...
00000014 0+0014 0+0018 FDE cie=0+0000 pc=.*
  DW_CFA_advance_loc: 4 to .*
  DW_CFA_remember_state
  DW_CFA_advance_loc: 4 to .*
  DW_CFA_restore_state
#...
0000002c 0+001[48] 0+0030 FDE cie=0+0000 pc=.*
  DW_CFA_advance_loc: 4 to .*
  DW_CFA_def_cfa: r0( \([er]ax\)|) ofs 16
  DW_CFA_advance_loc: 4 to .*
  DW_CFA_def_cfa_offset: 0
#pass
