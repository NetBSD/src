#as: --64 -madd-bnd-prefix
#ld: -shared -melf_x86_64 -z bndplt
#objdump: -dw

#...
0+2d0 <.plt>:
[ 	]*[a-f0-9]+:	ff 35 7a 01 20 00    	pushq  0x20017a\(%rip\)        # 200450 <_GLOBAL_OFFSET_TABLE_\+0x8>
[ 	]*[a-f0-9]+:	f2 ff 25 7b 01 20 00 	bnd jmpq \*0x20017b\(%rip\)        # 200458 <_GLOBAL_OFFSET_TABLE_\+0x10>
[ 	]*[a-f0-9]+:	0f 1f 00             	nopl   \(%rax\)
[ 	]*[a-f0-9]+:	68 03 00 00 00       	pushq  \$0x3
[ 	]*[a-f0-9]+:	f2 e9 e5 ff ff ff    	bnd jmpq 2d0 <\*ABS\*\+0x34c@plt-0x50>
[ 	]*[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)
[ 	]*[a-f0-9]+:	68 00 00 00 00       	pushq  \$0x0
[ 	]*[a-f0-9]+:	f2 e9 d5 ff ff ff    	bnd jmpq 2d0 <\*ABS\*\+0x34c@plt-0x50>
[ 	]*[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)
[ 	]*[a-f0-9]+:	68 01 00 00 00       	pushq  \$0x1
[ 	]*[a-f0-9]+:	f2 e9 c5 ff ff ff    	bnd jmpq 2d0 <\*ABS\*\+0x34c@plt-0x50>
[ 	]*[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)
[ 	]*[a-f0-9]+:	68 02 00 00 00       	pushq  \$0x2
[ 	]*[a-f0-9]+:	f2 e9 b5 ff ff ff    	bnd jmpq 2d0 <\*ABS\*\+0x34c@plt-0x50>
[ 	]*[a-f0-9]+:	0f 1f 44 00 00       	nopl   0x0\(%rax,%rax,1\)

Disassembly of section .plt.bnd:

0+320 <\*ABS\*\+0x34c@plt>:
[ 	]*[a-f0-9]+:	f2 ff 25 39 01 20 00 	bnd jmpq \*0x200139\(%rip\)        # 200460 <_GLOBAL_OFFSET_TABLE_\+0x18>
[ 	]*[a-f0-9]+:	90                   	nop

0+328 <func1@plt>:
[ 	]*[a-f0-9]+:	f2 ff 25 39 01 20 00 	bnd jmpq \*0x200139\(%rip\)        # 200468 <_GLOBAL_OFFSET_TABLE_\+0x20>
[ 	]*[a-f0-9]+:	90                   	nop

0+330 <func2@plt>:
[ 	]*[a-f0-9]+:	f2 ff 25 39 01 20 00 	bnd jmpq \*0x200139\(%rip\)        # 200470 <_GLOBAL_OFFSET_TABLE_\+0x28>
[ 	]*[a-f0-9]+:	90                   	nop

0+338 <\*ABS\*\+0x340@plt>:
[ 	]*[a-f0-9]+:	f2 ff 25 39 01 20 00 	bnd jmpq \*0x200139\(%rip\)        # 200478 <_GLOBAL_OFFSET_TABLE_\+0x30>
[ 	]*[a-f0-9]+:	90                   	nop

Disassembly of section .text:

0+340 <resolve1>:
[ 	]*[a-f0-9]+:	f2 e8 e2 ff ff ff    	bnd callq 328 <func1@plt>

0+346 <g1>:
[ 	]*[a-f0-9]+:	f2 e9 ec ff ff ff    	bnd jmpq 338 <\*ABS\*\+0x340@plt>

0+34c <resolve2>:
[ 	]*[a-f0-9]+:	f2 e8 de ff ff ff    	bnd callq 330 <func2@plt>

0+352 <g2>:
[ 	]*[a-f0-9]+:	f2 e9 c8 ff ff ff    	bnd jmpq 320 <\*ABS\*\+0x34c@plt>
#pass
