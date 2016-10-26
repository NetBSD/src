#source: ../x86-64-branch.s
#as: -J
#objdump: -drw -Mintel64
#name: x86-64 (ILP32) branch

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
[ 	]*[a-f0-9]+:	e8 00 00 00 00       	callq  0x1f	1b: R_X86_64_PC32	\*ABS\*\+0x10003c
[ 	]*[a-f0-9]+:	e9 00 00 00 00       	jmpq   0x24	20: R_X86_64_PC32	\*ABS\*\+0x10003c
[ 	]*[a-f0-9]+:	66 e8 00 00 00 00    	data16 callq 0x2a	26: R_X86_64_PC32	foo-0x4
[ 	]*[a-f0-9]+:	66 e9 00 00 00 00    	data16 jmpq 0x30	2c: R_X86_64_PC32	foo-0x4
[ 	]*[a-f0-9]+:	66 0f 82 00 00 00 00 	data16 jb 0x37	33: R_X86_64_PC32	foo-0x4
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
[ 	]*[a-f0-9]+:	e8 00 00 00 00       	callq  0x56	52: R_X86_64_PC32	\*ABS\*\+0x10003c
[ 	]*[a-f0-9]+:	e9 00 00 00 00       	jmpq   0x5b	57: R_X86_64_PC32	\*ABS\*\+0x10003c
#pass
