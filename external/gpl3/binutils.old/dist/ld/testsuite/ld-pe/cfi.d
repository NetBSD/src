#source: cfia.s
#source: cfib.s
#ld: --file-align 1 --section-align 1
#objdump: -Wf

#...
00000004 00000014 ffffffff CIE
  Version:               1
  Augmentation:          ""
  Code alignment factor: 1
  Data alignment factor: \-8
  Return address column: 32

  DW_CFA_def_cfa: r7 \(rsp\) ofs 8
  DW_CFA_offset: r32 \(xmm15\) at cfa\-8
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0000001c 00000024 00000004 FDE cie=00000004 pc=.*
  DW_CFA_advance_loc: 4 to .*
  DW_CFA_def_cfa_offset: 16
  DW_CFA_offset: r6 \(rbp\) at cfa\-16
  DW_CFA_advance_loc: 4 to .*
^  DW_CFA_def_cfa: r7 \(rsp\) ofs 8
  DW_CFA_restore: r6 \(rbp\)
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
#pass
