#objdump: -dwMsuffix
#name: x86-64 stack-related opcodes (with suffixes)
#source: x86-64-stack.s

.*: +file format .*

Disassembly of section .text:

0+ <_start>:
[ 	]*[a-f0-9]+:	50                   	pushq  %rax
[ 	]*[a-f0-9]+:	66 50                	pushw  %ax
[ 	]*[a-f0-9]+:	66 48 50             	data32 pushq %rax
[ 	]*[a-f0-9]+:	58                   	popq   %rax
[ 	]*[a-f0-9]+:	66 58                	popw   %ax
[ 	]*[a-f0-9]+:	66 48 58             	data32 popq %rax
[ 	]*[a-f0-9]+:	8f c0                	popq   %rax
[ 	]*[a-f0-9]+:	66 8f c0             	popw   %ax
[ 	]*[a-f0-9]+:	66 48 8f c0          	data32 popq %rax
[ 	]*[a-f0-9]+:	8f 00                	popq   \(%rax\)
[ 	]*[a-f0-9]+:	66 8f 00             	popw   \(%rax\)
[ 	]*[a-f0-9]+:	66 48 8f 00          	data32 popq \(%rax\)
[ 	]*[a-f0-9]+:	ff d0                	callq  \*%rax
[ 	]*[a-f0-9]+:	66 ff d0             	callw  \*%ax
[ 	]*[a-f0-9]+:	66 48 ff d0          	data32 callq \*%rax
[ 	]*[a-f0-9]+:	ff 10                	callq  \*\(%rax\)
[ 	]*[a-f0-9]+:	66 ff 10             	callw  \*\(%rax\)
[ 	]*[a-f0-9]+:	66 48 ff 10          	data32 callq \*\(%rax\)
[ 	]*[a-f0-9]+:	ff e0                	jmpq   \*%rax
[ 	]*[a-f0-9]+:	66 ff e0             	jmpw   \*%ax
[ 	]*[a-f0-9]+:	66 48 ff e0          	data32 jmpq \*%rax
[ 	]*[a-f0-9]+:	ff 20                	jmpq   \*\(%rax\)
[ 	]*[a-f0-9]+:	66 ff 20             	jmpw   \*\(%rax\)
[ 	]*[a-f0-9]+:	66 48 ff 20          	data32 jmpq \*\(%rax\)
[ 	]*[a-f0-9]+:	ff f0                	pushq  %rax
[ 	]*[a-f0-9]+:	66 ff f0             	pushw  %ax
[ 	]*[a-f0-9]+:	66 48 ff f0          	data32 pushq %rax
[ 	]*[a-f0-9]+:	ff 30                	pushq  \(%rax\)
[ 	]*[a-f0-9]+:	66 ff 30             	pushw  \(%rax\)
[ 	]*[a-f0-9]+:	66 48 ff 30          	data32 pushq \(%rax\)
#pass
