#objdump: -dwr

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
[ 	]*[a-f0-9]+:	b8 00 00 00 00       	mov    \$0x0,%eax	1: R_386_GOT32	foo
[ 	]*[a-f0-9]+:	8b 05 00 00 00 00    	mov    0x0,%eax	7: R_386_GOT32X	foo
[ 	]*[a-f0-9]+:	8b 80 00 00 00 00    	mov    0x0\(%eax\),%eax	d: R_386_GOT32X	foo
[ 	]*[a-f0-9]+:	05 00 00 00 00       	add    \$0x0,%eax	12: R_386_GOT32	foo
[ 	]*[a-f0-9]+:	03 05 00 00 00 00    	add    0x0,%eax	18: R_386_GOT32X	foo
[ 	]*[a-f0-9]+:	03 80 00 00 00 00    	add    0x0\(%eax\),%eax	1e: R_386_GOT32X	foo
[ 	]*[a-f0-9]+:	ff 15 00 00 00 00    	call   \*0x0	24: R_386_GOT32X	foo
[ 	]*[a-f0-9]+:	ff 90 00 00 00 00    	call   \*0x0\(%eax\)	2a: R_386_GOT32X	foo
[ 	]*[a-f0-9]+:	ff 25 00 00 00 00    	jmp    \*0x0	30: R_386_GOT32X	foo
[ 	]*[a-f0-9]+:	ff a0 00 00 00 00    	jmp    \*0x0\(%eax\)	36: R_386_GOT32X	foo
[ 	]*[a-f0-9]+:	b8 00 00 00 00       	mov    \$0x0,%eax	3b: R_386_GOT32	foo
[ 	]*[a-f0-9]+:	8b 05 00 00 00 00    	mov    0x0,%eax	41: R_386_GOT32X	foo
[ 	]*[a-f0-9]+:	8b 80 00 00 00 00    	mov    0x0\(%eax\),%eax	47: R_386_GOT32X	foo
[ 	]*[a-f0-9]+:	05 00 00 00 00       	add    \$0x0,%eax	4c: R_386_GOT32	foo
[ 	]*[a-f0-9]+:	03 05 00 00 00 00    	add    0x0,%eax	52: R_386_GOT32X	foo
[ 	]*[a-f0-9]+:	03 80 00 00 00 00    	add    0x0\(%eax\),%eax	58: R_386_GOT32X	foo
[ 	]*[a-f0-9]+:	ff 90 00 00 00 00    	call   \*0x0\(%eax\)	5e: R_386_GOT32X	foo
[ 	]*[a-f0-9]+:	ff 15 00 00 00 00    	call   \*0x0	64: R_386_GOT32X	foo
[ 	]*[a-f0-9]+:	ff a0 00 00 00 00    	jmp    \*0x0\(%eax\)	6a: R_386_GOT32X	foo
[ 	]*[a-f0-9]+:	ff 25 00 00 00 00    	jmp    \*0x0	70: R_386_GOT32X	foo
#pass
