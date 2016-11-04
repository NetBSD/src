#objdump: -Wf
#name: CFI common 4
#...
Contents of the .eh_frame section:

00000000 0+0010 0+0000 CIE
  Version:               1
  Augmentation:          "zR"
  Code alignment factor: .*
  Data alignment factor: .*
  Return address column: .*
  Augmentation data:     [01]b
#...
00000014 0+0010 0+0018 FDE cie=0+0000 pc=.*
  DW_CFA_remember_state
  DW_CFA_restore_state
#...
00000028 0+001[04] 0+002c FDE cie=0+0000 pc=.*
  DW_CFA_remember_state
  DW_CFA_restore_state
#pass
