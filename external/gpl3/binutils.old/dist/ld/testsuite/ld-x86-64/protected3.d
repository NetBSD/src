#as: --64
#ld: -shared -melf_x86_64
#objdump: -drw

.*: +file format .*


Disassembly of section .text:

0+[a-f0-9]+ <bar>:
[ 	]*[a-f0-9]+:	48 8b 05 ([0-9a-f]{2} ){4} *	mov    0x[a-f0-9]+\(%rip\),%rax        # [a-f0-9]+ <_DYNAMIC\+0x[a-f0-9]+>
[ 	]*[a-f0-9]+:	8b 00                	mov    \(%rax\),%eax
[ 	]*[a-f0-9]+:	c3                   	retq *
#pass
