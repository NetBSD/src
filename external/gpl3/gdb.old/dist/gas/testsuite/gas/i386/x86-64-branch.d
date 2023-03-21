#as: -J
#objdump: -dw -Mintel64
#name: x86-64 branch

.*: +file format .*

Disassembly of section .text:

0+ <.text>:
[ 	]*[a-f0-9]+:	ff d0                	call   \*%rax
[ 	]*[a-f0-9]+:	ff d0                	call   \*%rax
[ 	]*[a-f0-9]+:	66 ff d0             	data16 call \*%rax
[ 	]*[a-f0-9]+:	66 ff d0             	data16 call \*%rax
[ 	]*[a-f0-9]+:	66 ff 10             	data16 call \*\(%rax\)
[ 	]*[a-f0-9]+:	ff e0                	jmp    \*%rax
[ 	]*[a-f0-9]+:	ff e0                	jmp    \*%rax
[ 	]*[a-f0-9]+:	66 ff e0             	data16 jmp \*%rax
[ 	]*[a-f0-9]+:	66 ff e0             	data16 jmp \*%rax
[ 	]*[a-f0-9]+:	66 ff 20             	data16 jmp \*\(%rax\)
[ 	]*[a-f0-9]+:	e8 (00|5b) 00 (00|10) 00       	call   (0x1f|10007a <.text\+0x10007a>)
[ 	]*[a-f0-9]+:	e9 (00|60) 00 (00|10) 00       	jmp    (0x24|100084 <.text\+0x100084>)
[ 	]*[a-f0-9]+:	66 e8 00 00 00 00    	data16 call (0x2a|2a <.text\+0x2a>)
[ 	]*[a-f0-9]+:	66 e9 00 00 00 00    	data16 jmp (0x30|30 <.text\+0x30>)
[ 	]*[a-f0-9]+:	66 0f 82 00 00 00 00 	data16 jb (0x37|37 <.text\+0x37>)
[ 	]*[a-f0-9]+:	66 c3                	data16 ret *
[ 	]*[a-f0-9]+:	66 c2 08 00          	data16 ret \$0x8
[ 	]*[a-f0-9]+:	ff d0                	call   \*%rax
[ 	]*[a-f0-9]+:	ff d0                	call   \*%rax
[ 	]*[a-f0-9]+:	66 ff d0             	data16 call \*%rax
[ 	]*[a-f0-9]+:	66 ff d0             	data16 call \*%rax
[ 	]*[a-f0-9]+:	66 ff 10             	data16 call \*\(%rax\)
[ 	]*[a-f0-9]+:	ff e0                	jmp    \*%rax
[ 	]*[a-f0-9]+:	ff e0                	jmp    \*%rax
[ 	]*[a-f0-9]+:	66 ff e0             	data16 jmp \*%rax
[ 	]*[a-f0-9]+:	66 ff e0             	data16 jmp \*%rax
[ 	]*[a-f0-9]+:	66 ff 20             	data16 jmp \*\(%rax\)
[ 	]*[a-f0-9]+:	e8 .. 00 (00|10) 00       	call   (0x[0-9a-f]*|100[0-9a-f]* <.text\+0x100[0-9a-f]*>)
[ 	]*[a-f0-9]+:	e9 .. 00 (00|10) 00       	jmp    (0x[0-9a-f]*|100[0-9a-f]* <.text\+0x100[0-9a-f]*>)
[ 	]*[a-f0-9]+:	66 c3                	data16 ret *
[ 	]*[a-f0-9]+:	66 c2 08 00          	data16 ret \$0x8
#pass
