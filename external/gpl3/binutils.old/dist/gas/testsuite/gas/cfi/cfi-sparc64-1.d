#readelf: -wf
#name: CFI on SPARC 64-bit
#as: -64

Contents of the .eh_frame section:

00000000 00000014 00000000 CIE
  Version:               1
  Augmentation:          "zR"
  Code alignment factor: 4
  Data alignment factor: -8
  Return address column: 15
  Augmentation data:     1b

  DW_CFA_def_cfa: r14 ofs 2047
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000018 00000014 0000001c FDE cie=00000000 pc=00000000..00000030
  DW_CFA_advance_loc: 4 to 00000004
  DW_CFA_def_cfa_register: r30
  DW_CFA_GNU_window_save
  DW_CFA_register: r15 in r31

