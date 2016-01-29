#source: pr17154-x86.s
#as: --64
#ld: -shared -melf_x86_64
#objdump: -dw
#target: x86_64-*-*

#...
0+2d0 <\*ABS\*\+0x32a@plt-0x10>:
[ 	]*[a-f0-9]+:	ff 35 5a 01 20 00    	pushq  0x20015a\(%rip\)        # 200430 <_GLOBAL_OFFSET_TABLE_\+0x8>
[ 	]*[a-f0-9]+:	ff 25 5c 01 20 00    	jmpq   \*0x20015c\(%rip\)        # 200438 <_GLOBAL_OFFSET_TABLE_\+0x10>
[ 	]*[a-f0-9]+:	0f 1f 40 00          	nopl   0x0\(%rax\)

0+2e0 <\*ABS\*\+0x32a@plt>:
[ 	]*[a-f0-9]+:	ff 25 5a 01 20 00    	jmpq   \*0x20015a\(%rip\)        # 200440 <_GLOBAL_OFFSET_TABLE_\+0x18>
[ 	]*[a-f0-9]+:	68 03 00 00 00       	pushq  \$0x3
[ 	]*[a-f0-9]+:	e9 e0 ff ff ff       	jmpq   2d0 <\*ABS\*\+0x32a@plt-0x10>

0+2f0 <func1@plt>:
[ 	]*[a-f0-9]+:	ff 25 52 01 20 00    	jmpq   \*0x200152\(%rip\)        # 200448 <_GLOBAL_OFFSET_TABLE_\+0x20>
[ 	]*[a-f0-9]+:	68 00 00 00 00       	pushq  \$0x0
[ 	]*[a-f0-9]+:	e9 d0 ff ff ff       	jmpq   2d0 <\*ABS\*\+0x32a@plt-0x10>

0+300 <func2@plt>:
[ 	]*[a-f0-9]+:	ff 25 4a 01 20 00    	jmpq   \*0x20014a\(%rip\)        # 200450 <_GLOBAL_OFFSET_TABLE_\+0x28>
[ 	]*[a-f0-9]+:	68 01 00 00 00       	pushq  \$0x1
[ 	]*[a-f0-9]+:	e9 c0 ff ff ff       	jmpq   2d0 <\*ABS\*\+0x32a@plt-0x10>

0+310 <\*ABS\*\+0x320@plt>:
[ 	]*[a-f0-9]+:	ff 25 42 01 20 00    	jmpq   \*0x200142\(%rip\)        # 200458 <_GLOBAL_OFFSET_TABLE_\+0x30>
[ 	]*[a-f0-9]+:	68 02 00 00 00       	pushq  \$0x2
[ 	]*[a-f0-9]+:	e9 b0 ff ff ff       	jmpq   2d0 <\*ABS\*\+0x32a@plt-0x10>

Disassembly of section .text:

0+320 <resolve1>:
[ 	]*[a-f0-9]+:	e8 cb ff ff ff       	callq  2f0 <func1@plt>

0+325 <g1>:
[ 	]*[a-f0-9]+:	e9 e6 ff ff ff       	jmpq   310 <\*ABS\*\+0x320@plt>

0+32a <resolve2>:
[ 	]*[a-f0-9]+:	e8 d1 ff ff ff       	callq  300 <func2@plt>

0+32f <g2>:
[ 	]*[a-f0-9]+:	e9 ac ff ff ff       	jmpq   2e0 <\*ABS\*\+0x32a@plt>
#pass
