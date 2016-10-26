#source: pr17154-x86.s
#ld: -m elf_i386 -shared
#as: --32
#objdump: -dw
#target: x86_64-*-* i?86-*-*

#...
0+1d0 <\*ABS\*@plt-0x10>:
[ 	]*[a-f0-9]+:	ff b3 04 00 00 00    	pushl  0x4\(%ebx\)
[ 	]*[a-f0-9]+:	ff a3 08 00 00 00    	jmp    \*0x8\(%ebx\)
[ 	]*[a-f0-9]+:	00 00                	add    %al,\(%eax\)
	...

0+1e0 <\*ABS\*@plt>:
[ 	]*[a-f0-9]+:	ff a3 0c 00 00 00    	jmp    \*0xc\(%ebx\)
[ 	]*[a-f0-9]+:	68 18 00 00 00       	push   \$0x18
[ 	]*[a-f0-9]+:	e9 e0 ff ff ff       	jmp    1d0 <\*ABS\*@plt-0x10>

0+1f0 <func1@plt>:
[ 	]*[a-f0-9]+:	ff a3 10 00 00 00    	jmp    \*0x10\(%ebx\)
[ 	]*[a-f0-9]+:	68 00 00 00 00       	push   \$0x0
[ 	]*[a-f0-9]+:	e9 d0 ff ff ff       	jmp    1d0 <\*ABS\*@plt-0x10>

0+200 <func2@plt>:
[ 	]*[a-f0-9]+:	ff a3 14 00 00 00    	jmp    \*0x14\(%ebx\)
[ 	]*[a-f0-9]+:	68 08 00 00 00       	push   \$0x8
[ 	]*[a-f0-9]+:	e9 c0 ff ff ff       	jmp    1d0 <\*ABS\*@plt-0x10>

0+210 <\*ABS\*@plt>:
[ 	]*[a-f0-9]+:	ff a3 18 00 00 00    	jmp    \*0x18\(%ebx\)
[ 	]*[a-f0-9]+:	68 10 00 00 00       	push   \$0x10
[ 	]*[a-f0-9]+:	e9 b0 ff ff ff       	jmp    1d0 <\*ABS\*@plt-0x10>

Disassembly of section .text:

0+220 <resolve1>:
[ 	]*[a-f0-9]+:	e8 cb ff ff ff       	call   1f0 <func1@plt>

0+225 <g1>:
[ 	]*[a-f0-9]+:	e9 e6 ff ff ff       	jmp    210 <\*ABS\*@plt>

0+22a <resolve2>:
[ 	]*[a-f0-9]+:	e8 d1 ff ff ff       	call   200 <func2@plt>

0+22f <g2>:
[ 	]*[a-f0-9]+:	e9 ac ff ff ff       	jmp    1e0 <\*ABS\*@plt>
#pass
