#source: load1.s
#as: --x32
#ld: -shared -melf32_x86_64
#objdump: -dw
#target: x86_64-*-nacl*

.*: +file format .*

Disassembly of section .text:

0+ <_start>:
[ 	]*[a-f0-9]+:	13 05 fa 01 01 10    	adc    0x100101fa\(%rip\),%eax        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	03 1d f4 01 01 10    	add    0x100101f4\(%rip\),%ebx        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	23 0d ee 01 01 10    	and    0x100101ee\(%rip\),%ecx        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	3b 15 e8 01 01 10    	cmp    0x100101e8\(%rip\),%edx        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	0b 35 e2 01 01 10    	or     0x100101e2\(%rip\),%esi        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	1b 3d dc 01 01 10    	sbb    0x100101dc\(%rip\),%edi        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	2b 2d d6 01 01 10    	sub    0x100101d6\(%rip\),%ebp        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	44 33 05 cf 01 01 10 	xor    0x100101cf\(%rip\),%r8d        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	44 85 3d c8 01 01 10 	test   %r15d,0x100101c8\(%rip\)        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	48 13 05 c1 01 01 10 	adc    0x100101c1\(%rip\),%rax        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	48 03 1d ba 01 01 10 	add    0x100101ba\(%rip\),%rbx        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	48 23 0d b3 01 01 10 	and    0x100101b3\(%rip\),%rcx        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	48 3b 15 ac 01 01 10 	cmp    0x100101ac\(%rip\),%rdx        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	48 0b 3d a5 01 01 10 	or     0x100101a5\(%rip\),%rdi        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	48 1b 35 9e 01 01 10 	sbb    0x1001019e\(%rip\),%rsi        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	48 2b 2d 97 01 01 10 	sub    0x10010197\(%rip\),%rbp        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	4c 33 05 90 01 01 10 	xor    0x10010190\(%rip\),%r8        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	4c 85 3d 89 01 01 10 	test   %r15,0x10010189\(%rip\)        # 10010200 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	13 05 8b 01 01 10    	adc    0x1001018b\(%rip\),%eax        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	03 1d 85 01 01 10    	add    0x10010185\(%rip\),%ebx        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	23 0d 7f 01 01 10    	and    0x1001017f\(%rip\),%ecx        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	3b 15 79 01 01 10    	cmp    0x10010179\(%rip\),%edx        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	0b 35 73 01 01 10    	or     0x10010173\(%rip\),%esi        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	1b 3d 6d 01 01 10    	sbb    0x1001016d\(%rip\),%edi        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	2b 2d 67 01 01 10    	sub    0x10010167\(%rip\),%ebp        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	44 33 05 60 01 01 10 	xor    0x10010160\(%rip\),%r8d        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	44 85 3d 59 01 01 10 	test   %r15d,0x10010159\(%rip\)        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	48 13 05 52 01 01 10 	adc    0x10010152\(%rip\),%rax        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	48 03 1d 4b 01 01 10 	add    0x1001014b\(%rip\),%rbx        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	48 23 0d 44 01 01 10 	and    0x10010144\(%rip\),%rcx        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	48 3b 15 3d 01 01 10 	cmp    0x1001013d\(%rip\),%rdx        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	48 0b 3d 36 01 01 10 	or     0x10010136\(%rip\),%rdi        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	48 1b 35 2f 01 01 10 	sbb    0x1001012f\(%rip\),%rsi        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	48 2b 2d 28 01 01 10 	sub    0x10010128\(%rip\),%rbp        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	4c 33 05 21 01 01 10 	xor    0x10010121\(%rip\),%r8        # 10010208 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	4c 85 3d 1a 01 01 10 	test   %r15,0x1001011a\(%rip\)        # 10010208 <_DYNAMIC\+0x78>
#pass
