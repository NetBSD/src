#source: bnd-branch-1.s -mx86-used-note=no
#as: --64
#ld: -z now -shared -melf_x86_64 --hash-style=sysv -z max-page-size=0x200000 -z noseparate-code
#objdump: -dw

.*: +file format .*


Disassembly of section .plt:

0+230 <.plt>:
 +[a-f0-9]+:	ff 35 82 01 20 00    	push   0x200182\(%rip\)        # 2003b8 <_GLOBAL_OFFSET_TABLE_\+0x8>
 +[a-f0-9]+:	ff 25 84 01 20 00    	jmp    \*0x200184\(%rip\)        # 2003c0 <_GLOBAL_OFFSET_TABLE_\+0x10>
 +[a-f0-9]+:	0f 1f 40 00          	nopl   0x0\(%rax\)

0+240 <foo2@plt>:
 +[a-f0-9]+:	ff 25 82 01 20 00    	jmp    \*0x200182\(%rip\)        # 2003c8 <foo2>
 +[a-f0-9]+:	68 00 00 00 00       	push   \$0x0
 +[a-f0-9]+:	e9 e0 ff ff ff       	jmp    230 <.plt>

0+250 <foo3@plt>:
 +[a-f0-9]+:	ff 25 7a 01 20 00    	jmp    \*0x20017a\(%rip\)        # 2003d0 <foo3>
 +[a-f0-9]+:	68 01 00 00 00       	push   \$0x1
 +[a-f0-9]+:	e9 d0 ff ff ff       	jmp    230 <.plt>

0+260 <foo1@plt>:
 +[a-f0-9]+:	ff 25 72 01 20 00    	jmp    \*0x200172\(%rip\)        # 2003d8 <foo1>
 +[a-f0-9]+:	68 02 00 00 00       	push   \$0x2
 +[a-f0-9]+:	e9 c0 ff ff ff       	jmp    230 <.plt>

0+270 <foo4@plt>:
 +[a-f0-9]+:	ff 25 6a 01 20 00    	jmp    \*0x20016a\(%rip\)        # 2003e0 <foo4>
 +[a-f0-9]+:	68 03 00 00 00       	push   \$0x3
 +[a-f0-9]+:	e9 b0 ff ff ff       	jmp    230 <.plt>

Disassembly of section .text:

0+280 <_start>:
 +[a-f0-9]+:	f2 e9 da ff ff ff    	bnd jmp 260 <foo1@plt>
 +[a-f0-9]+:	e8 b5 ff ff ff       	call   240 <foo2@plt>
 +[a-f0-9]+:	e9 c0 ff ff ff       	jmp    250 <foo3@plt>
 +[a-f0-9]+:	e8 db ff ff ff       	call   270 <foo4@plt>
 +[a-f0-9]+:	f2 e8 b5 ff ff ff    	bnd call 250 <foo3@plt>
 +[a-f0-9]+:	e9 d0 ff ff ff       	jmp    270 <foo4@plt>
#pass
