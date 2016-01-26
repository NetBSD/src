#objdump: -Wf
#name: CFI on i386
#...
Contents of the .eh_frame section:

00000000 00000014 00000000 CIE
  Version:               1
  Augmentation:          "zR"
  Code alignment factor: 1
  Data alignment factor: -4
  Return address column: 8
  Augmentation data:     1b

  DW_CFA_def_cfa: r4 \(esp\) ofs 4
  DW_CFA_offset: r8 \(eip\) at cfa-4
  DW_CFA_nop
  DW_CFA_nop

00000018 00000014 0000001c FDE cie=00000000 pc=00000000..00000012
  DW_CFA_advance_loc: 6 to 00000006
  DW_CFA_def_cfa_offset: 4664
  DW_CFA_advance_loc: 11 to 00000011
  DW_CFA_def_cfa_offset: 4

00000030 00000018 00000034 FDE cie=00000000 pc=00000012..0000001f
  DW_CFA_advance_loc: 1 to 00000013
  DW_CFA_def_cfa_offset: 8
  DW_CFA_offset: r5 \(ebp\) at cfa-8
  DW_CFA_advance_loc: 2 to 00000015
  DW_CFA_def_cfa_register: r5 \(ebp\)
  DW_CFA_advance_loc: 9 to 0000001e
  DW_CFA_def_cfa_register: r4 \(esp\)

0000004c 00000014 00000050 FDE cie=00000000 pc=0000001f..0000002f
  DW_CFA_advance_loc: 2 to 00000021
  DW_CFA_def_cfa_register: r3 \(ebx\)
  DW_CFA_advance_loc: 13 to 0000002e
  DW_CFA_def_cfa: r4 \(esp\) ofs 4

00000064 00000010 00000068 FDE cie=00000000 pc=0000002f..00000035
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

00000078 00000010 0000007c FDE cie=00000000 pc=00000035..00000044
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0000008c 00000010 00000000 CIE
  Version:               1
  Augmentation:          "zR"
  Code alignment factor: 1
  Data alignment factor: -4
  Return address column: 8
  Augmentation data:     1b

  DW_CFA_undefined: r8 \(eip\)
  DW_CFA_nop

000000a0 00000094 00000018 FDE cie=0000008c pc=00000044..00000071
  DW_CFA_advance_loc: 1 to 00000045
  DW_CFA_undefined: r0 \(eax\)
  DW_CFA_advance_loc: 1 to 00000046
  DW_CFA_undefined: r1 \(ecx\)
  DW_CFA_advance_loc: 1 to 00000047
  DW_CFA_undefined: r2 \(edx\)
  DW_CFA_advance_loc: 1 to 00000048
  DW_CFA_undefined: r3 \(ebx\)
  DW_CFA_advance_loc: 1 to 00000049
  DW_CFA_undefined: r4 \(esp\)
  DW_CFA_advance_loc: 1 to 0000004a
  DW_CFA_undefined: r5 \(ebp\)
  DW_CFA_advance_loc: 1 to 0000004b
  DW_CFA_undefined: r6 \(esi\)
  DW_CFA_advance_loc: 1 to 0000004c
  DW_CFA_undefined: r7 \(edi\)
  DW_CFA_advance_loc: 1 to 0000004d
  DW_CFA_undefined: r9 \(eflags\)
  DW_CFA_advance_loc: 1 to 0000004e
  DW_CFA_undefined: r40 \(es\)
  DW_CFA_advance_loc: 1 to 0000004f
  DW_CFA_undefined: r41 \(cs\)
  DW_CFA_advance_loc: 1 to 00000050
  DW_CFA_undefined: r43 \(ds\)
  DW_CFA_advance_loc: 1 to 00000051
  DW_CFA_undefined: r42 \(ss\)
  DW_CFA_advance_loc: 1 to 00000052
  DW_CFA_undefined: r44 \(fs\)
  DW_CFA_advance_loc: 1 to 00000053
  DW_CFA_undefined: r45 \(gs\)
  DW_CFA_advance_loc: 1 to 00000054
  DW_CFA_undefined: r48 \(tr\)
  DW_CFA_advance_loc: 1 to 00000055
  DW_CFA_undefined: r49 \(ldtr\)
  DW_CFA_advance_loc: 1 to 00000056
  DW_CFA_undefined: r39 \(mxcsr\)
  DW_CFA_advance_loc: 1 to 00000057
  DW_CFA_undefined: r21 \(xmm0\)
  DW_CFA_advance_loc: 1 to 00000058
  DW_CFA_undefined: r22 \(xmm1\)
  DW_CFA_advance_loc: 1 to 00000059
  DW_CFA_undefined: r23 \(xmm2\)
  DW_CFA_advance_loc: 1 to 0000005a
  DW_CFA_undefined: r24 \(xmm3\)
  DW_CFA_advance_loc: 1 to 0000005b
  DW_CFA_undefined: r25 \(xmm4\)
  DW_CFA_advance_loc: 1 to 0000005c
  DW_CFA_undefined: r26 \(xmm5\)
  DW_CFA_advance_loc: 1 to 0000005d
  DW_CFA_undefined: r27 \(xmm6\)
  DW_CFA_advance_loc: 1 to 0000005e
  DW_CFA_undefined: r28 \(xmm7\)
  DW_CFA_advance_loc: 1 to 0000005f
  DW_CFA_undefined: r37 \(fcw\)
  DW_CFA_advance_loc: 1 to 00000060
  DW_CFA_undefined: r38 \(fsw\)
  DW_CFA_advance_loc: 1 to 00000061
  DW_CFA_undefined: r11 \(st\(?0?\)?\)
  DW_CFA_advance_loc: 1 to 00000062
  DW_CFA_undefined: r12 \(st\(?1\)?\)
  DW_CFA_advance_loc: 1 to 00000063
  DW_CFA_undefined: r13 \(st\(?2\)?\)
  DW_CFA_advance_loc: 1 to 00000064
  DW_CFA_undefined: r14 \(st\(?3\)?\)
  DW_CFA_advance_loc: 1 to 00000065
  DW_CFA_undefined: r15 \(st\(?4\)?\)
  DW_CFA_advance_loc: 1 to 00000066
  DW_CFA_undefined: r16 \(st\(?5\)?\)
  DW_CFA_advance_loc: 1 to 00000067
  DW_CFA_undefined: r17 \(st\(?6\)?\)
  DW_CFA_advance_loc: 1 to 00000068
  DW_CFA_undefined: r18 \(st\(?7\)?\)
  DW_CFA_advance_loc: 1 to 00000069
  DW_CFA_undefined: r29 \(mm0\)
  DW_CFA_advance_loc: 1 to 0000006a
  DW_CFA_undefined: r30 \(mm1\)
  DW_CFA_advance_loc: 1 to 0000006b
  DW_CFA_undefined: r31 \(mm2\)
  DW_CFA_advance_loc: 1 to 0000006c
  DW_CFA_undefined: r32 \(mm3\)
  DW_CFA_advance_loc: 1 to 0000006d
  DW_CFA_undefined: r33 \(mm4\)
  DW_CFA_advance_loc: 1 to 0000006e
  DW_CFA_undefined: r34 \(mm5\)
  DW_CFA_advance_loc: 1 to 0000006f
  DW_CFA_undefined: r35 \(mm6\)
  DW_CFA_advance_loc: 1 to 00000070
  DW_CFA_undefined: r36 \(mm7\)
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

