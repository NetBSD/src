#as:
#objdump: -drw
#name: x86-64 32bit displacement

.*: +file format .*


Disassembly of section .text:

0+ <.*>:
[ 	]*[a-f0-9]+:	8b 58 03             	mov    0x3\(%rax\),%ebx
[ 	]*[a-f0-9]+:	8b 98 03 00 00 00    	mov    0x3\(%rax\),%ebx
[ 	]*[a-f0-9]+:	eb 05                	jmp    10 <foo>
[ 	]*[a-f0-9]+:	e9 00 00 00 00       	jmpq   10 <foo>

0+10 <foo>:
[ 	]*[a-f0-9]+:	89 58 03             	mov    %ebx,0x3\(%rax\)
[ 	]*[a-f0-9]+:	89 98 03 00 00 00    	mov    %ebx,0x3\(%rax\)
#pass
