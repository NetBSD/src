#as: --64 -mrelax-relocations=yes
#ld: -melf_x86_64
#objdump: -dw
#target: x86_64-*-*

.*: +file format .*

#...
0+4000e0 <__start>:
[ 	]*[a-f0-9]+:	ff 15 42 00 20 00    	callq  \*0x200042\(%rip\)        # 600128 <_GLOBAL_OFFSET_TABLE_\+0x18>
[ 	]*[a-f0-9]+:	ff 25 3c 00 20 00    	jmpq   \*0x20003c\(%rip\)        # 600128 <_GLOBAL_OFFSET_TABLE_\+0x18>
[ 	]*[a-f0-9]+:	48 03 05 35 00 20 00 	add    0x200035\(%rip\),%rax        # 600128 <_GLOBAL_OFFSET_TABLE_\+0x18>
[ 	]*[a-f0-9]+:	48 8b 05 2e 00 20 00 	mov    0x20002e\(%rip\),%rax        # 600128 <_GLOBAL_OFFSET_TABLE_\+0x18>
[ 	]*[a-f0-9]+:	48 85 05 27 00 20 00 	test   %rax,0x200027\(%rip\)        # 600128 <_GLOBAL_OFFSET_TABLE_\+0x18>
[ 	]*[a-f0-9]+:	48 8d ([0-9a-f]{2} ){5}[ 	]+lea[ 	]+.*
#pass
