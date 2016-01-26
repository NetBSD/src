#readelf: -wf
#name: CFI on SPARC 32-bit
#as: -32

Contents of the .eh_frame section:

00000000 00000010 00000000 CIE
  Version:               1
  Augmentation:          "zR"
  Code alignment factor: 4
  Data alignment factor: -4
  Return address column: 15
  Augmentation data:     1b

  DW_CFA_def_cfa: r14 ofs 0

00000014 00000014 00000018 FDE cie=00000000 pc=00000000..00000024
  DW_CFA_advance_loc: 4 to 00000004
  DW_CFA_def_cfa_register: r30
  DW_CFA_GNU_window_save
  DW_CFA_register: r15 in r31

