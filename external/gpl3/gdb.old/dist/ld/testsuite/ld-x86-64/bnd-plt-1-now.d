#source: bnd-branch-1.s
#as: --64 -mx86-used-note=no
#ld: -z now -shared -melf_x86_64 -z bndplt --hash-style=sysv -z max-page-size=0x200000 -z noseparate-code
#objdump: -dw

.*: +file format .*


Disassembly of section .plt:

0+230 <.plt>:
 +[a-f0-9]+:	ff 35 a2 01 20 00    	push   0x2001a2\(%rip\)        # 2003d8 <_GLOBAL_OFFSET_TABLE_\+0x8>
 +[a-f0-9]+:	f2 ff 25 a3 01 20 00 	bnd jmp \*0x2001a3\(%rip\)        # 2003e0 <_GLOBAL_OFFSET_TABLE_\+0x10>
 +[a-f0-9]+:	0f 1f 00             	nopl   \(%rax\)
 +[a-f0-9]+:	68 00 00 00 00       	push   \$0x0
 +[a-f0-9]+:	f2 e9 e5 ff ff ff    	bnd jmp 230 <.plt>
 +[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)
 +[a-f0-9]+:	68 01 00 00 00       	push   \$0x1
 +[a-f0-9]+:	f2 e9 d5 ff ff ff    	bnd jmp 230 <.plt>
 +[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)
 +[a-f0-9]+:	68 02 00 00 00       	push   \$0x2
 +[a-f0-9]+:	f2 e9 c5 ff ff ff    	bnd jmp 230 <.plt>
 +[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)
 +[a-f0-9]+:	68 03 00 00 00       	push   \$0x3
 +[a-f0-9]+:	f2 e9 b5 ff ff ff    	bnd jmp 230 <.plt>
 +[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)

Disassembly of section .plt.sec:

0+280 <foo2@plt>:
 +[a-f0-9]+:	f2 ff 25 61 01 20 00 	bnd jmp \*0x200161\(%rip\)        # 2003e8 <foo2>
 +[a-f0-9]+:	90                   	nop

0+288 <foo3@plt>:
 +[a-f0-9]+:	f2 ff 25 61 01 20 00 	bnd jmp \*0x200161\(%rip\)        # 2003f0 <foo3>
 +[a-f0-9]+:	90                   	nop

0+290 <foo1@plt>:
 +[a-f0-9]+:	f2 ff 25 61 01 20 00 	bnd jmp \*0x200161\(%rip\)        # 2003f8 <foo1>
 +[a-f0-9]+:	90                   	nop

0+298 <foo4@plt>:
 +[a-f0-9]+:	f2 ff 25 61 01 20 00 	bnd jmp \*0x200161\(%rip\)        # 200400 <foo4>
 +[a-f0-9]+:	90                   	nop

Disassembly of section .text:

0+2a0 <_start>:
 +[a-f0-9]+:	f2 e9 ea ff ff ff    	bnd jmp 290 <foo1@plt>
 +[a-f0-9]+:	e8 d5 ff ff ff       	call   280 <foo2@plt>
 +[a-f0-9]+:	e9 d8 ff ff ff       	jmp    288 <foo3@plt>
 +[a-f0-9]+:	e8 e3 ff ff ff       	call   298 <foo4@plt>
 +[a-f0-9]+:	f2 e8 cd ff ff ff    	bnd call 288 <foo3@plt>
 +[a-f0-9]+:	e9 d8 ff ff ff       	jmp    298 <foo4@plt>
#pass
