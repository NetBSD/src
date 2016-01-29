#as: -J -madd-bnd-prefix
#objdump: -dwr
#name: x86-64 branch with BND prefix

.*: +file format .*


Disassembly of section .text:

0+ <foo1-0xc>:
[ 	]*[a-f0-9]+:	f2 e8 00 00 00 00    	bnd callq 6 <foo1-0x6>	2: R_X86_64_PC32	\*ABS\*\+0x10003c
[ 	]*[a-f0-9]+:	f2 e9 00 00 00 00    	bnd jmpq c <foo1>	8: R_X86_64_PC32	\*ABS\*\+0x10003c

0+c <foo1>:
[ 	]*[a-f0-9]+:	f2 eb fd             	bnd jmp c <foo1>
[ 	]*[a-f0-9]+:	f2 72 fa             	bnd jb c <foo1>
[ 	]*[a-f0-9]+:	f2 e8 f4 ff ff ff    	bnd callq c <foo1>
[ 	]*[a-f0-9]+:	f2 eb 09             	bnd jmp 24 <foo2>
[ 	]*[a-f0-9]+:	f2 72 06             	bnd jb 24 <foo2>
[ 	]*[a-f0-9]+:	f2 e8 00 00 00 00    	bnd callq 24 <foo2>

0+24 <foo2>:
[ 	]*[a-f0-9]+:	f2 e9 00 00 00 00    	bnd jmpq 2a <foo2\+0x6>	26: R_X86_64_PC32	foo-0x4
[ 	]*[a-f0-9]+:	f2 0f 82 00 00 00 00 	bnd jb 31 <foo2\+0xd>	2d: R_X86_64_PC32	foo-0x4
[ 	]*[a-f0-9]+:	f2 e8 00 00 00 00    	bnd callq 37 <foo2\+0x13>	33: R_X86_64_PC32	foo-0x4
[ 	]*[a-f0-9]+:	f2 e9 00 00 00 00    	bnd jmpq 3d <foo2\+0x19>	39: R_X86_64_PLT32	foo-0x4
[ 	]*[a-f0-9]+:	f2 0f 82 00 00 00 00 	bnd jb 44 <foo2\+0x20>	40: R_X86_64_PLT32	foo-0x4
[ 	]*[a-f0-9]+:	f2 e8 00 00 00 00    	bnd callq 4a <foo2\+0x26>	46: R_X86_64_PLT32	foo-0x4
