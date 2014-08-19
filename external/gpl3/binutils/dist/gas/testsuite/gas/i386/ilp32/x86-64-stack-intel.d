#source: ../x86-64-stack.s
#objdump: -dwMintel
#name: x86-64 (ILP32) stack-related opcodes (Intel mode)

.*: +file format .*

Disassembly of section .text:

0+ <_start>:
[ 	]*[a-f0-9]+:	50                   	push   rax
[ 	]*[a-f0-9]+:	66 50                	push   ax
[ 	]*[a-f0-9]+:	66 48 50             	data32 push rax
[ 	]*[a-f0-9]+:	58                   	pop    rax
[ 	]*[a-f0-9]+:	66 58                	pop    ax
[ 	]*[a-f0-9]+:	66 48 58             	data32 pop rax
[ 	]*[a-f0-9]+:	8f c0                	pop    rax
[ 	]*[a-f0-9]+:	66 8f c0             	pop    ax
[ 	]*[a-f0-9]+:	66 48 8f c0          	data32 pop rax
[ 	]*[a-f0-9]+:	8f 00                	pop    QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 8f 00             	pop    WORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 48 8f 00          	data32 pop QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	ff d0                	call   rax
[ 	]*[a-f0-9]+:	66 ff d0             	call   ax
[ 	]*[a-f0-9]+:	66 48 ff d0          	data32 call rax
[ 	]*[a-f0-9]+:	ff 10                	call   QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 ff 10             	call   WORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 48 ff 10          	data32 call QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	ff e0                	jmp    rax
[ 	]*[a-f0-9]+:	66 ff e0             	jmp    ax
[ 	]*[a-f0-9]+:	66 48 ff e0          	data32 jmp rax
[ 	]*[a-f0-9]+:	ff 20                	jmp    QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 ff 20             	jmp    WORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 48 ff 20          	data32 jmp QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	ff f0                	push   rax
[ 	]*[a-f0-9]+:	66 ff f0             	push   ax
[ 	]*[a-f0-9]+:	66 48 ff f0          	data32 push rax
[ 	]*[a-f0-9]+:	ff 30                	push   QWORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 ff 30             	push   WORD PTR \[rax\]
[ 	]*[a-f0-9]+:	66 48 ff 30          	data32 push QWORD PTR \[rax\]
#pass
