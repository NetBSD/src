#source: load1.s
#as: --x32
#ld: -melf32_x86_64
#objdump: -dw --sym
#notarget: x86_64-*-nacl*

.*: +file format .*

SYMBOL TABLE:
#...
0+60017c l     O .data	0+1 bar
#...
0+60017d g     O .data	0+1 foo
#...


Disassembly of section .text:

0+400074 <_start>:
[ 	]*[a-f0-9]+:	81 d0 7c 01 60 00    	adc    \$0x60017c,%eax
[ 	]*[a-f0-9]+:	81 c3 7c 01 60 00    	add    \$0x60017c,%ebx
[ 	]*[a-f0-9]+:	81 e1 7c 01 60 00    	and    \$0x60017c,%ecx
[ 	]*[a-f0-9]+:	81 fa 7c 01 60 00    	cmp    \$0x60017c,%edx
[ 	]*[a-f0-9]+:	81 ce 7c 01 60 00    	or     \$0x60017c,%esi
[ 	]*[a-f0-9]+:	81 df 7c 01 60 00    	sbb    \$0x60017c,%edi
[ 	]*[a-f0-9]+:	81 ed 7c 01 60 00    	sub    \$0x60017c,%ebp
[ 	]*[a-f0-9]+:	41 81 f0 7c 01 60 00 	xor    \$0x60017c,%r8d
[ 	]*[a-f0-9]+:	41 f7 c7 7c 01 60 00 	test   \$0x60017c,%r15d
[ 	]*[a-f0-9]+:	48 81 d0 7c 01 60 00 	adc    \$0x60017c,%rax
[ 	]*[a-f0-9]+:	48 81 c3 7c 01 60 00 	add    \$0x60017c,%rbx
[ 	]*[a-f0-9]+:	48 81 e1 7c 01 60 00 	and    \$0x60017c,%rcx
[ 	]*[a-f0-9]+:	48 81 fa 7c 01 60 00 	cmp    \$0x60017c,%rdx
[ 	]*[a-f0-9]+:	48 81 cf 7c 01 60 00 	or     \$0x60017c,%rdi
[ 	]*[a-f0-9]+:	48 81 de 7c 01 60 00 	sbb    \$0x60017c,%rsi
[ 	]*[a-f0-9]+:	48 81 ed 7c 01 60 00 	sub    \$0x60017c,%rbp
[ 	]*[a-f0-9]+:	49 81 f0 7c 01 60 00 	xor    \$0x60017c,%r8
[ 	]*[a-f0-9]+:	49 f7 c7 7c 01 60 00 	test   \$0x60017c,%r15
[ 	]*[a-f0-9]+:	81 d0 7d 01 60 00    	adc    \$0x60017d,%eax
[ 	]*[a-f0-9]+:	81 c3 7d 01 60 00    	add    \$0x60017d,%ebx
[ 	]*[a-f0-9]+:	81 e1 7d 01 60 00    	and    \$0x60017d,%ecx
[ 	]*[a-f0-9]+:	81 fa 7d 01 60 00    	cmp    \$0x60017d,%edx
[ 	]*[a-f0-9]+:	81 ce 7d 01 60 00    	or     \$0x60017d,%esi
[ 	]*[a-f0-9]+:	81 df 7d 01 60 00    	sbb    \$0x60017d,%edi
[ 	]*[a-f0-9]+:	81 ed 7d 01 60 00    	sub    \$0x60017d,%ebp
[ 	]*[a-f0-9]+:	41 81 f0 7d 01 60 00 	xor    \$0x60017d,%r8d
[ 	]*[a-f0-9]+:	41 f7 c7 7d 01 60 00 	test   \$0x60017d,%r15d
[ 	]*[a-f0-9]+:	48 81 d0 7d 01 60 00 	adc    \$0x60017d,%rax
[ 	]*[a-f0-9]+:	48 81 c3 7d 01 60 00 	add    \$0x60017d,%rbx
[ 	]*[a-f0-9]+:	48 81 e1 7d 01 60 00 	and    \$0x60017d,%rcx
[ 	]*[a-f0-9]+:	48 81 fa 7d 01 60 00 	cmp    \$0x60017d,%rdx
[ 	]*[a-f0-9]+:	48 81 cf 7d 01 60 00 	or     \$0x60017d,%rdi
[ 	]*[a-f0-9]+:	48 81 de 7d 01 60 00 	sbb    \$0x60017d,%rsi
[ 	]*[a-f0-9]+:	48 81 ed 7d 01 60 00 	sub    \$0x60017d,%rbp
[ 	]*[a-f0-9]+:	49 81 f0 7d 01 60 00 	xor    \$0x60017d,%r8
[ 	]*[a-f0-9]+:	49 f7 c7 7d 01 60 00 	test   \$0x60017d,%r15
#pass
