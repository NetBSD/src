#source: eh1.s
#source: eh2a.s
#as: --64
#ld: -melf_x86_64 -Ttext 0x400078
#readelf: -wf
#target: x86_64-*-*

Contents of the .eh_frame section:

00000000 00000014 00000000 CIE
  Version:               1
  Augmentation:          ""
  Code alignment factor: 1
  Data alignment factor: -8
  Return address column: 16

  DW_CFA_def_cfa: r7 \(rsp\) ofs 8
  DW_CFA_offset: r16 \(rip\) at cfa-8
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000018 0000001c 0000001c FDE cie=00000000 pc=00400078..00400078
  DW_CFA_advance_loc: 0 to 00400078
  DW_CFA_def_cfa_offset: 16
  DW_CFA_offset: r6 \(rbp\) at cfa-16
  DW_CFA_advance_loc: 0 to 00400078
  DW_CFA_def_cfa_register: r6 \(rbp\)

00000038 ZERO terminator

