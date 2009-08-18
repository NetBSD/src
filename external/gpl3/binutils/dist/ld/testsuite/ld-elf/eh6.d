#source: eh6.s
#ld: --gc-sections -shared
#readelf: -wf
#target: x86_64-*-linux-gnu i?86-*-linux-gnu

The section .eh_frame contains:

00000000 0000001[4c] 00000000 CIE
  Version:               1
  Augmentation:          "zPR"
  Code alignment factor: 1
  Data alignment factor: .*
  Return address column: .*
  Augmentation data:     80 .* 1b

  DW_CFA_nop
#pass
