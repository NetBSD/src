#source: load1.s
#as: --64 -mrelax-relocations=yes
#ld: -melf_x86_64
#objdump: -dw --sym
#notarget: x86_64-*-nacl*

.*: +file format .*

SYMBOL TABLE:
#...
0+6001b8 l     O .data	0+1 bar
#...
0+6001b9 g     O .data	0+1 foo
#...

Disassembly of section .text:

0+4000b0 <_start>:
[ 	]*[a-f0-9]+:	81 d0 b8 01 60 00    	adc    \$0x6001b8,%eax
[ 	]*[a-f0-9]+:	81 c3 b8 01 60 00    	add    \$0x6001b8,%ebx
[ 	]*[a-f0-9]+:	81 e1 b8 01 60 00    	and    \$0x6001b8,%ecx
[ 	]*[a-f0-9]+:	81 fa b8 01 60 00    	cmp    \$0x6001b8,%edx
[ 	]*[a-f0-9]+:	81 ce b8 01 60 00    	or     \$0x6001b8,%esi
[ 	]*[a-f0-9]+:	81 df b8 01 60 00    	sbb    \$0x6001b8,%edi
[ 	]*[a-f0-9]+:	81 ed b8 01 60 00    	sub    \$0x6001b8,%ebp
[ 	]*[a-f0-9]+:	41 81 f0 b8 01 60 00 	xor    \$0x6001b8,%r8d
[ 	]*[a-f0-9]+:	41 f7 c7 b8 01 60 00 	test   \$0x6001b8,%r15d
[ 	]*[a-f0-9]+:	48 81 d0 b8 01 60 00 	adc    \$0x6001b8,%rax
[ 	]*[a-f0-9]+:	48 81 c3 b8 01 60 00 	add    \$0x6001b8,%rbx
[ 	]*[a-f0-9]+:	48 81 e1 b8 01 60 00 	and    \$0x6001b8,%rcx
[ 	]*[a-f0-9]+:	48 81 fa b8 01 60 00 	cmp    \$0x6001b8,%rdx
[ 	]*[a-f0-9]+:	48 81 cf b8 01 60 00 	or     \$0x6001b8,%rdi
[ 	]*[a-f0-9]+:	48 81 de b8 01 60 00 	sbb    \$0x6001b8,%rsi
[ 	]*[a-f0-9]+:	48 81 ed b8 01 60 00 	sub    \$0x6001b8,%rbp
[ 	]*[a-f0-9]+:	49 81 f0 b8 01 60 00 	xor    \$0x6001b8,%r8
[ 	]*[a-f0-9]+:	49 f7 c7 b8 01 60 00 	test   \$0x6001b8,%r15
[ 	]*[a-f0-9]+:	81 d0 b9 01 60 00    	adc    \$0x6001b9,%eax
[ 	]*[a-f0-9]+:	81 c3 b9 01 60 00    	add    \$0x6001b9,%ebx
[ 	]*[a-f0-9]+:	81 e1 b9 01 60 00    	and    \$0x6001b9,%ecx
[ 	]*[a-f0-9]+:	81 fa b9 01 60 00    	cmp    \$0x6001b9,%edx
[ 	]*[a-f0-9]+:	81 ce b9 01 60 00    	or     \$0x6001b9,%esi
[ 	]*[a-f0-9]+:	81 df b9 01 60 00    	sbb    \$0x6001b9,%edi
[ 	]*[a-f0-9]+:	81 ed b9 01 60 00    	sub    \$0x6001b9,%ebp
[ 	]*[a-f0-9]+:	41 81 f0 b9 01 60 00 	xor    \$0x6001b9,%r8d
[ 	]*[a-f0-9]+:	41 f7 c7 b9 01 60 00 	test   \$0x6001b9,%r15d
[ 	]*[a-f0-9]+:	48 81 d0 b9 01 60 00 	adc    \$0x6001b9,%rax
[ 	]*[a-f0-9]+:	48 81 c3 b9 01 60 00 	add    \$0x6001b9,%rbx
[ 	]*[a-f0-9]+:	48 81 e1 b9 01 60 00 	and    \$0x6001b9,%rcx
[ 	]*[a-f0-9]+:	48 81 fa b9 01 60 00 	cmp    \$0x6001b9,%rdx
[ 	]*[a-f0-9]+:	48 81 cf b9 01 60 00 	or     \$0x6001b9,%rdi
[ 	]*[a-f0-9]+:	48 81 de b9 01 60 00 	sbb    \$0x6001b9,%rsi
[ 	]*[a-f0-9]+:	48 81 ed b9 01 60 00 	sub    \$0x6001b9,%rbp
[ 	]*[a-f0-9]+:	49 81 f0 b9 01 60 00 	xor    \$0x6001b9,%r8
[ 	]*[a-f0-9]+:	49 f7 c7 b9 01 60 00 	test   \$0x6001b9,%r15
#pass
