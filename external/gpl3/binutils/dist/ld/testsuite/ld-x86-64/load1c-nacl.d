#source: load1.s
#as: --64
#ld: -shared -melf_x86_64
#objdump: -dw
#target: x86_64-*-nacl*

.*: +file format .*

Disassembly of section .text:

0+ <_start>:
[ 	]*[a-f0-9]+:	13 05 22 03 01 10    	adc    0x10010322\(%rip\),%eax        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	03 1d 1c 03 01 10    	add    0x1001031c\(%rip\),%ebx        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	23 0d 16 03 01 10    	and    0x10010316\(%rip\),%ecx        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	3b 15 10 03 01 10    	cmp    0x10010310\(%rip\),%edx        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	0b 35 0a 03 01 10    	or     0x1001030a\(%rip\),%esi        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	1b 3d 04 03 01 10    	sbb    0x10010304\(%rip\),%edi        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	2b 2d fe 02 01 10    	sub    0x100102fe\(%rip\),%ebp        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	44 33 05 f7 02 01 10 	xor    0x100102f7\(%rip\),%r8d        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	44 85 3d f0 02 01 10 	test   %r15d,0x100102f0\(%rip\)        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	48 13 05 e9 02 01 10 	adc    0x100102e9\(%rip\),%rax        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	48 03 1d e2 02 01 10 	add    0x100102e2\(%rip\),%rbx        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	48 23 0d db 02 01 10 	and    0x100102db\(%rip\),%rcx        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	48 3b 15 d4 02 01 10 	cmp    0x100102d4\(%rip\),%rdx        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	48 0b 3d cd 02 01 10 	or     0x100102cd\(%rip\),%rdi        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	48 1b 35 c6 02 01 10 	sbb    0x100102c6\(%rip\),%rsi        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	48 2b 2d bf 02 01 10 	sub    0x100102bf\(%rip\),%rbp        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	4c 33 05 b8 02 01 10 	xor    0x100102b8\(%rip\),%r8        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	4c 85 3d b1 02 01 10 	test   %r15,0x100102b1\(%rip\)        # 10010328 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	13 05 b3 02 01 10    	adc    0x100102b3\(%rip\),%eax        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	03 1d ad 02 01 10    	add    0x100102ad\(%rip\),%ebx        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	23 0d a7 02 01 10    	and    0x100102a7\(%rip\),%ecx        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	3b 15 a1 02 01 10    	cmp    0x100102a1\(%rip\),%edx        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	0b 35 9b 02 01 10    	or     0x1001029b\(%rip\),%esi        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	1b 3d 95 02 01 10    	sbb    0x10010295\(%rip\),%edi        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	2b 2d 8f 02 01 10    	sub    0x1001028f\(%rip\),%ebp        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	44 33 05 88 02 01 10 	xor    0x10010288\(%rip\),%r8d        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	44 85 3d 81 02 01 10 	test   %r15d,0x10010281\(%rip\)        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	48 13 05 7a 02 01 10 	adc    0x1001027a\(%rip\),%rax        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	48 03 1d 73 02 01 10 	add    0x10010273\(%rip\),%rbx        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	48 23 0d 6c 02 01 10 	and    0x1001026c\(%rip\),%rcx        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	48 3b 15 65 02 01 10 	cmp    0x10010265\(%rip\),%rdx        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	48 0b 3d 5e 02 01 10 	or     0x1001025e\(%rip\),%rdi        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	48 1b 35 57 02 01 10 	sbb    0x10010257\(%rip\),%rsi        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	48 2b 2d 50 02 01 10 	sub    0x10010250\(%rip\),%rbp        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	4c 33 05 49 02 01 10 	xor    0x10010249\(%rip\),%r8        # 10010330 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	4c 85 3d 42 02 01 10 	test   %r15,0x10010242\(%rip\)        # 10010330 <_DYNAMIC\+0xe8>
#pass
