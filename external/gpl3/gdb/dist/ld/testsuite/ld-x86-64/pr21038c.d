#name: PR ld/21038 (.plt.got and .plt.sec)
#as: --64
#ld: -z bndplt -melf_x86_64 -shared -z relro --ld-generated-unwind-info --hash-style=sysv -z max-page-size=0x200000 -z noseparate-code
#objdump: -dw -Wf

.*: +file format .*

Contents of the .eh_frame section:

0+ 0000000000000014 00000000 CIE
  Version:               1
  Augmentation:          "zR"
  Code alignment factor: 1
  Data alignment factor: -8
  Return address column: 16
  Augmentation data:     1b

  DW_CFA_def_cfa: r7 \(rsp\) ofs 8
  DW_CFA_offset: r16 \(rip\) at cfa-8
  DW_CFA_nop
  DW_CFA_nop

0+18 0000000000000014 0000001c FDE cie=00000000 pc=0000000000000220..0000000000000231
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+30 0000000000000024 00000034 FDE cie=00000000 pc=00000000000001f0..0000000000000210
  DW_CFA_def_cfa_offset: 16
  DW_CFA_advance_loc: 6 to 00000000000001f6
  DW_CFA_def_cfa_offset: 24
  DW_CFA_advance_loc: 10 to 0000000000000200
  DW_CFA_def_cfa_expression \(DW_OP_breg7 \(rsp\): 8; DW_OP_breg16 \(rip\): 0; DW_OP_lit15; DW_OP_and; DW_OP_lit5; DW_OP_ge; DW_OP_lit3; DW_OP_shl; DW_OP_plus\)
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+58 0000000000000014 0000005c FDE cie=00000000 pc=0000000000000210..0000000000000218
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop

0+70 0000000000000010 00000074 FDE cie=00000000 pc=0000000000000218..0000000000000220
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop


Disassembly of section .plt:

0+1f0 <.plt>:
 +[a-f0-9]+:	ff 35 12 0e 20 00    	push   0x200e12\(%rip\)        # 201008 <_GLOBAL_OFFSET_TABLE_\+0x8>
 +[a-f0-9]+:	f2 ff 25 13 0e 20 00 	bnd jmp \*0x200e13\(%rip\)        # 201010 <_GLOBAL_OFFSET_TABLE_\+0x10>
 +[a-f0-9]+:	0f 1f 00             	nopl   \(%rax\)
 +[a-f0-9]+:	68 00 00 00 00       	push   \$0x0
 +[a-f0-9]+:	f2 e9 e5 ff ff ff    	bnd jmp 1f0 <.plt>
 +[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)

Disassembly of section .plt.got:

0+210 <func1@plt>:
 +[a-f0-9]+:	f2 ff 25 e1 0d 20 00 	bnd jmp \*0x200de1\(%rip\)        # 200ff8 <func1>
 +[a-f0-9]+:	90                   	nop

Disassembly of section .plt.sec:

0+218 <func2@plt>:
 +[a-f0-9]+:	f2 ff 25 f9 0d 20 00 	bnd jmp \*0x200df9\(%rip\)        # 201018 <func2>
 +[a-f0-9]+:	90                   	nop

Disassembly of section .text:

0+220 <foo>:
 +[a-f0-9]+:	e8 eb ff ff ff       	call   210 <func1@plt>
 +[a-f0-9]+:	e8 ee ff ff ff       	call   218 <func2@plt>
 +[a-f0-9]+:	48 8b 05 c7 0d 20 00 	mov    0x200dc7\(%rip\),%rax        # 200ff8 <func1>
#pass
