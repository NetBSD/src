#source: bnd-ifunc-1.s
#as: --64 -madd-bnd-prefix -mx86-used-note=no
#ld: -z now -shared -melf_x86_64 -z bndplt --hash-style=sysv -z max-page-size=0x200000 -z noseparate-code
#objdump: -dw

.*: +file format .*


Disassembly of section .plt:

0+170 <.plt>:
 +[a-f0-9]+:	ff 35 4a 01 20 00    	push   0x20014a\(%rip\)        # 2002c0 <_GLOBAL_OFFSET_TABLE_\+0x8>
 +[a-f0-9]+:	f2 ff 25 4b 01 20 00 	bnd jmp \*0x20014b\(%rip\)        # 2002c8 <_GLOBAL_OFFSET_TABLE_\+0x10>
 +[a-f0-9]+:	0f 1f 00             	nopl   \(%rax\)
 +[a-f0-9]+:	68 00 00 00 00       	push   \$0x0
 +[a-f0-9]+:	f2 e9 e5 ff ff ff    	bnd jmp 170 <.plt>
 +[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)

Disassembly of section .plt.sec:

0+190 <\*ABS\*\+0x198@plt>:
 +[a-f0-9]+:	f2 ff 25 39 01 20 00 	bnd jmp \*0x200139\(%rip\)        # 2002d0 <_GLOBAL_OFFSET_TABLE_\+0x18>
 +[a-f0-9]+:	90                   	nop

Disassembly of section .text:

0+198 <foo>:
 +[a-f0-9]+:	f2 c3                	bnd ret *

0+19a <bar>:
 +[a-f0-9]+:	f2 e8 f0 ff ff ff    	bnd call 190 <\*ABS\*\+0x198@plt>
 +[a-f0-9]+:	f2 c3                	bnd ret *
#pass
