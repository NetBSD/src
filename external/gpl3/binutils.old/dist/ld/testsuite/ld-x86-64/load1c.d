#source: load1.s
#as: --64
#ld: -shared -melf_x86_64
#objdump: -dw
#notarget: x86_64-*-nacl*

.*: +file format .*

Disassembly of section .text:

[a-f0-9]+ <_start>:
[ 	]*[a-f0-9]+:	13 05 ca 01 20 00    	adc    0x2001ca\(%rip\),%eax        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	03 1d c4 01 20 00    	add    0x2001c4\(%rip\),%ebx        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	23 0d be 01 20 00    	and    0x2001be\(%rip\),%ecx        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	3b 15 b8 01 20 00    	cmp    0x2001b8\(%rip\),%edx        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	0b 35 b2 01 20 00    	or     0x2001b2\(%rip\),%esi        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	1b 3d ac 01 20 00    	sbb    0x2001ac\(%rip\),%edi        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	2b 2d a6 01 20 00    	sub    0x2001a6\(%rip\),%ebp        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	44 33 05 9f 01 20 00 	xor    0x20019f\(%rip\),%r8d        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	44 85 3d 98 01 20 00 	test   %r15d,0x200198\(%rip\)        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	48 13 05 91 01 20 00 	adc    0x200191\(%rip\),%rax        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	48 03 1d 8a 01 20 00 	add    0x20018a\(%rip\),%rbx        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	48 23 0d 83 01 20 00 	and    0x200183\(%rip\),%rcx        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	48 3b 15 7c 01 20 00 	cmp    0x20017c\(%rip\),%rdx        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	48 0b 3d 75 01 20 00 	or     0x200175\(%rip\),%rdi        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	48 1b 35 6e 01 20 00 	sbb    0x20016e\(%rip\),%rsi        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	48 2b 2d 67 01 20 00 	sub    0x200167\(%rip\),%rbp        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	4c 33 05 60 01 20 00 	xor    0x200160\(%rip\),%r8        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	4c 85 3d 59 01 20 00 	test   %r15,0x200159\(%rip\)        # 2003e0 <_DYNAMIC\+0xe0>
[ 	]*[a-f0-9]+:	13 05 5b 01 20 00    	adc    0x20015b\(%rip\),%eax        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	03 1d 55 01 20 00    	add    0x200155\(%rip\),%ebx        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	23 0d 4f 01 20 00    	and    0x20014f\(%rip\),%ecx        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	3b 15 49 01 20 00    	cmp    0x200149\(%rip\),%edx        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	0b 35 43 01 20 00    	or     0x200143\(%rip\),%esi        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	1b 3d 3d 01 20 00    	sbb    0x20013d\(%rip\),%edi        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	2b 2d 37 01 20 00    	sub    0x200137\(%rip\),%ebp        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	44 33 05 30 01 20 00 	xor    0x200130\(%rip\),%r8d        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	44 85 3d 29 01 20 00 	test   %r15d,0x200129\(%rip\)        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	48 13 05 22 01 20 00 	adc    0x200122\(%rip\),%rax        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	48 03 1d 1b 01 20 00 	add    0x20011b\(%rip\),%rbx        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	48 23 0d 14 01 20 00 	and    0x200114\(%rip\),%rcx        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	48 3b 15 0d 01 20 00 	cmp    0x20010d\(%rip\),%rdx        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	48 0b 3d 06 01 20 00 	or     0x200106\(%rip\),%rdi        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	48 1b 35 ff 00 20 00 	sbb    0x2000ff\(%rip\),%rsi        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	48 2b 2d f8 00 20 00 	sub    0x2000f8\(%rip\),%rbp        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	4c 33 05 f1 00 20 00 	xor    0x2000f1\(%rip\),%r8        # 2003e8 <_DYNAMIC\+0xe8>
[ 	]*[a-f0-9]+:	4c 85 3d ea 00 20 00 	test   %r15,0x2000ea\(%rip\)        # 2003e8 <_DYNAMIC\+0xe8>
#pass
