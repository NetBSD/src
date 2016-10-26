#source: load1.s
#as: --x32
#ld: -shared -melf32_x86_64
#objdump: -dw
#notarget: x86_64-*-nacl*

.*: +file format .*

Disassembly of section .text:

0+[a-f0-9]+ <_start>:
[ 	]*[a-f0-9]+:	13 05 5e 01 20 00    	adc    0x20015e\(%rip\),%eax        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	03 1d 58 01 20 00    	add    0x200158\(%rip\),%ebx        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	23 0d 52 01 20 00    	and    0x200152\(%rip\),%ecx        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	3b 15 4c 01 20 00    	cmp    0x20014c\(%rip\),%edx        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	0b 35 46 01 20 00    	or     0x200146\(%rip\),%esi        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	1b 3d 40 01 20 00    	sbb    0x200140\(%rip\),%edi        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	2b 2d 3a 01 20 00    	sub    0x20013a\(%rip\),%ebp        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	44 33 05 33 01 20 00 	xor    0x200133\(%rip\),%r8d        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	44 85 3d 2c 01 20 00 	test   %r15d,0x20012c\(%rip\)        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	48 13 05 25 01 20 00 	adc    0x200125\(%rip\),%rax        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	48 03 1d 1e 01 20 00 	add    0x20011e\(%rip\),%rbx        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	48 23 0d 17 01 20 00 	and    0x200117\(%rip\),%rcx        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	48 3b 15 10 01 20 00 	cmp    0x200110\(%rip\),%rdx        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	48 0b 3d 09 01 20 00 	or     0x200109\(%rip\),%rdi        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	48 1b 35 02 01 20 00 	sbb    0x200102\(%rip\),%rsi        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	48 2b 2d fb 00 20 00 	sub    0x2000fb\(%rip\),%rbp        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	4c 33 05 f4 00 20 00 	xor    0x2000f4\(%rip\),%r8        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	4c 85 3d ed 00 20 00 	test   %r15,0x2000ed\(%rip\)        # 2002d0 <_DYNAMIC\+0x70>
[ 	]*[a-f0-9]+:	13 05 ef 00 20 00    	adc    0x2000ef\(%rip\),%eax        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	03 1d e9 00 20 00    	add    0x2000e9\(%rip\),%ebx        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	23 0d e3 00 20 00    	and    0x2000e3\(%rip\),%ecx        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	3b 15 dd 00 20 00    	cmp    0x2000dd\(%rip\),%edx        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	0b 35 d7 00 20 00    	or     0x2000d7\(%rip\),%esi        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	1b 3d d1 00 20 00    	sbb    0x2000d1\(%rip\),%edi        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	2b 2d cb 00 20 00    	sub    0x2000cb\(%rip\),%ebp        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	44 33 05 c4 00 20 00 	xor    0x2000c4\(%rip\),%r8d        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	44 85 3d bd 00 20 00 	test   %r15d,0x2000bd\(%rip\)        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	48 13 05 b6 00 20 00 	adc    0x2000b6\(%rip\),%rax        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	48 03 1d af 00 20 00 	add    0x2000af\(%rip\),%rbx        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	48 23 0d a8 00 20 00 	and    0x2000a8\(%rip\),%rcx        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	48 3b 15 a1 00 20 00 	cmp    0x2000a1\(%rip\),%rdx        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	48 0b 3d 9a 00 20 00 	or     0x20009a\(%rip\),%rdi        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	48 1b 35 93 00 20 00 	sbb    0x200093\(%rip\),%rsi        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	48 2b 2d 8c 00 20 00 	sub    0x20008c\(%rip\),%rbp        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	4c 33 05 85 00 20 00 	xor    0x200085\(%rip\),%r8        # 2002d8 <_DYNAMIC\+0x78>
[ 	]*[a-f0-9]+:	4c 85 3d 7e 00 20 00 	test   %r15,0x20007e\(%rip\)        # 2002d8 <_DYNAMIC\+0x78>
#pass
