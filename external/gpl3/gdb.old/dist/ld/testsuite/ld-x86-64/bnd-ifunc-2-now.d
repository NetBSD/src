#source: bnd-ifunc-2.s
#as: --64 -madd-bnd-prefix -mx86-used-note=no
#ld: -z now -shared -melf_x86_64 -z bndplt --hash-style=sysv -z max-page-size=0x200000 -z noseparate-code
#objdump: -dw

.*: +file format .*


Disassembly of section .plt:

0+240 <.plt>:
 +[a-f0-9]+:	ff 35 9a 01 20 00    	push   0x20019a\(%rip\)        # 2003e0 <_GLOBAL_OFFSET_TABLE_\+0x8>
 +[a-f0-9]+:	f2 ff 25 9b 01 20 00 	bnd jmp \*0x20019b\(%rip\)        # 2003e8 <_GLOBAL_OFFSET_TABLE_\+0x10>
 +[a-f0-9]+:	0f 1f 00             	nopl   \(%rax\)
 +[a-f0-9]+:	68 03 00 00 00       	push   \$0x3
 +[a-f0-9]+:	f2 e9 e5 ff ff ff    	bnd jmp 240 <.plt>
 +[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)
 +[a-f0-9]+:	68 00 00 00 00       	push   \$0x0
 +[a-f0-9]+:	f2 e9 d5 ff ff ff    	bnd jmp 240 <.plt>
 +[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)
 +[a-f0-9]+:	68 01 00 00 00       	push   \$0x1
 +[a-f0-9]+:	f2 e9 c5 ff ff ff    	bnd jmp 240 <.plt>
 +[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)
 +[a-f0-9]+:	68 02 00 00 00       	push   \$0x2
 +[a-f0-9]+:	f2 e9 b5 ff ff ff    	bnd jmp 240 <.plt>
 +[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)

Disassembly of section .plt.sec:

0+290 <\*ABS\*\+0x2bc@plt>:
 +[a-f0-9]+:	f2 ff 25 59 01 20 00 	bnd jmp \*0x200159\(%rip\)        # 2003f0 <_GLOBAL_OFFSET_TABLE_\+0x18>
 +[a-f0-9]+:	90                   	nop

0+298 <func1@plt>:
 +[a-f0-9]+:	f2 ff 25 59 01 20 00 	bnd jmp \*0x200159\(%rip\)        # 2003f8 <func1>
 +[a-f0-9]+:	90                   	nop

0+2a0 <func2@plt>:
 +[a-f0-9]+:	f2 ff 25 59 01 20 00 	bnd jmp \*0x200159\(%rip\)        # 200400 <func2>
 +[a-f0-9]+:	90                   	nop

0+2a8 <\*ABS\*\+0x2b0@plt>:
 +[a-f0-9]+:	f2 ff 25 59 01 20 00 	bnd jmp \*0x200159\(%rip\)        # 200408 <_GLOBAL_OFFSET_TABLE_\+0x30>
 +[a-f0-9]+:	90                   	nop

Disassembly of section .text:

0+2b0 <resolve1>:
 +[a-f0-9]+:	f2 e8 e2 ff ff ff    	bnd call 298 <func1@plt>

0+2b6 <g1>:
 +[a-f0-9]+:	f2 e9 ec ff ff ff    	bnd jmp 2a8 <\*ABS\*\+0x2b0@plt>

0+2bc <resolve2>:
 +[a-f0-9]+:	f2 e8 de ff ff ff    	bnd call 2a0 <func2@plt>

0+2c2 <g2>:
 +[a-f0-9]+:	f2 e9 c8 ff ff ff    	bnd jmp 290 <\*ABS\*\+0x2bc@plt>
#pass
