#source: pr20244-2.s
#as: --32
#ld: -m elf_i386
#objdump: --sym -dw
#notarget: i?86-*-nacl* x86_64-*-nacl*

.*: +file format .*

SYMBOL TABLE:
#...
0+80480b1 l   i   .text	00000000 bar
#...
0+80480b2 g     F .text	00000000 _start
#...
0+80480b0 g   i   .text	00000000 foo
#...


Disassembly of section .plt:

0+8048090 <.plt>:
 +[a-f0-9]+:	ff 25 e0 90 04 08    	jmp    \*0x80490e0
 +[a-f0-9]+:	68 00 00 00 00       	push   \$0x0
 +[a-f0-9]+:	e9 00 00 00 00       	jmp    80480a0 <foo-0x10>
 +[a-f0-9]+:	ff 25 e4 90 04 08    	jmp    \*0x80490e4
 +[a-f0-9]+:	68 00 00 00 00       	push   \$0x0
 +[a-f0-9]+:	e9 00 00 00 00       	jmp    80480b0 <foo>

Disassembly of section .text:

0+80480b0 <foo>:
 +[a-f0-9]+:	c3                   	ret    

0+80480b1 <bar>:
 +[a-f0-9]+:	c3                   	ret    

0+80480b2 <_start>:
 +[a-f0-9]+:	ff 15 e0 90 04 08    	call   \*0x80490e0
 +[a-f0-9]+:	ff 25 e4 90 04 08    	jmp    \*0x80490e4
 +[a-f0-9]+:	c7 05 e4 90 04 08 00 00 00 00 	movl   \$0x0,0x80490e4
 +[a-f0-9]+:	83 3d e0 90 04 08 00 	cmpl   \$0x0,0x80490e0
 +[a-f0-9]+:	b9 10 00 00 00       	mov    \$0x10,%ecx
#pass
