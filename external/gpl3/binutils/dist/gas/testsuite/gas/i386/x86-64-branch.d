#as: -J
#objdump: -drw
#name: x86-64 indirect branch

.*: +file format .*

Disassembly of section .text:

0+ <.text>:
[ 	]*[a-f0-9]+:	ff d0                	callq  \*%rax
[ 	]*[a-f0-9]+:	ff d0                	callq  \*%rax
[ 	]*[a-f0-9]+:	66 ff d0             	callw  \*%ax
[ 	]*[a-f0-9]+:	66 ff d0             	callw  \*%ax
[ 	]*[a-f0-9]+:	66 ff 10             	callw  \*\(%rax\)
[ 	]*[a-f0-9]+:	ff e0                	jmpq   \*%rax
[ 	]*[a-f0-9]+:	ff e0                	jmpq   \*%rax
[ 	]*[a-f0-9]+:	66 ff e0             	jmpw   \*%ax
[ 	]*[a-f0-9]+:	66 ff e0             	jmpw   \*%ax
[ 	]*[a-f0-9]+:	66 ff 20             	jmpw   \*\(%rax\)
[ 	]*[a-f0-9]+:	ff d0                	callq  \*%rax
[ 	]*[a-f0-9]+:	ff d0                	callq  \*%rax
[ 	]*[a-f0-9]+:	66 ff d0             	callw  \*%ax
[ 	]*[a-f0-9]+:	66 ff d0             	callw  \*%ax
[ 	]*[a-f0-9]+:	66 ff 10             	callw  \*\(%rax\)
[ 	]*[a-f0-9]+:	ff e0                	jmpq   \*%rax
[ 	]*[a-f0-9]+:	ff e0                	jmpq   \*%rax
[ 	]*[a-f0-9]+:	66 ff e0             	jmpw   \*%ax
[ 	]*[a-f0-9]+:	66 ff e0             	jmpw   \*%ax
[ 	]*[a-f0-9]+:	66 ff 20             	jmpw   \*\(%rax\)
#pass
