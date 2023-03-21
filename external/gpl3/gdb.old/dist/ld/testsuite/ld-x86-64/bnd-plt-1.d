#source: bnd-branch-1.s -mx86-used-note=no
#as: --64
#ld: -shared -melf_x86_64 -z bndplt --hash-style=sysv -z max-page-size=0x200000 -z noseparate-code
#objdump: -dw

.*: +file format .*


Disassembly of section .plt:

0+230 <.plt>:
[ 	]*[a-f0-9]+:	ff 35 82 01 20 00    	push   0x200182\(%rip\)[ 	]*(#.*)?
[ 	]*[a-f0-9]+:	f2 ff 25 83 01 20 00 	bnd jmp \*0x200183\(%rip\)[ 	]*(#.*)?
[ 	]*[a-f0-9]+:	0f 1f 00             	nopl   \(%rax\)
[ 	]*[a-f0-9]+:	68 00 00 00 00       	push   \$0x0
[ 	]*[a-f0-9]+:	f2 e9 e5 ff ff ff    	bnd jmp 230 <.plt>
[ 	]*[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)
[ 	]*[a-f0-9]+:	68 01 00 00 00       	push   \$0x1
[ 	]*[a-f0-9]+:	f2 e9 d5 ff ff ff    	bnd jmp 230 <.plt>
[ 	]*[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)
[ 	]*[a-f0-9]+:	68 02 00 00 00       	push   \$0x2
[ 	]*[a-f0-9]+:	f2 e9 c5 ff ff ff    	bnd jmp 230 <.plt>
[ 	]*[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)
[ 	]*[a-f0-9]+:	68 03 00 00 00       	push   \$0x3
[ 	]*[a-f0-9]+:	f2 e9 b5 ff ff ff    	bnd jmp 230 <.plt>
[ 	]*[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)

Disassembly of section .plt.sec:

0+280 <foo2@plt>:
[ 	]*[a-f0-9]+:	f2 ff 25 41 01 20 00 	bnd jmp \*0x200141\(%rip\)[ 	]*(#.*)?
[ 	]*[a-f0-9]+:	90                   	nop

0+288 <foo3@plt>:
[ 	]*[a-f0-9]+:	f2 ff 25 41 01 20 00 	bnd jmp \*0x200141\(%rip\)[ 	]*(#.*)?
[ 	]*[a-f0-9]+:	90                   	nop

0+290 <foo1@plt>:
[ 	]*[a-f0-9]+:	f2 ff 25 41 01 20 00 	bnd jmp \*0x200141\(%rip\)[ 	]*(#.*)?
[ 	]*[a-f0-9]+:	90                   	nop

0+298 <foo4@plt>:
[ 	]*[a-f0-9]+:	f2 ff 25 41 01 20 00 	bnd jmp \*0x200141\(%rip\)[ 	]*(#.*)?
[ 	]*[a-f0-9]+:	90                   	nop

Disassembly of section .text:

0+2a0 <_start>:
[ 	]*[a-f0-9]+:	f2 e9 ea ff ff ff    	bnd jmp 290 <foo1@plt>
[ 	]*[a-f0-9]+:	e8 d5 ff ff ff       	call   280 <foo2@plt>
[ 	]*[a-f0-9]+:	e9 d8 ff ff ff       	jmp    288 <foo3@plt>
[ 	]*[a-f0-9]+:	e8 e3 ff ff ff       	call   298 <foo4@plt>
[ 	]*[a-f0-9]+:	f2 e8 cd ff ff ff    	bnd call 288 <foo3@plt>
[ 	]*[a-f0-9]+:	e9 d8 ff ff ff       	jmp    298 <foo4@plt>
#pass
