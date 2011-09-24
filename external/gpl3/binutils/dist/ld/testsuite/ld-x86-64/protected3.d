#as: --64
#ld: -shared -melf_x86_64
#objdump: -drw

.*: +file format .*


Disassembly of section .text:

0+[a-f0-9]+ <bar>:
[ 	]*[a-f0-9]+:	8b 05 [a-f0-9][a-f0-9] 00 [a-f0-9][a-f0-9] 00    	mov    0x[a-f0-9]+\(%rip\),%eax        # [a-f0-9]+ <foo>
[ 	]*[a-f0-9]+:	c3                   	retq   
#pass
