#as: -madd-bnd-prefix
#objdump: -drw
#name: Check -madd-bnd-prefix (x86-64)

.*: +file format .*


Disassembly of section .text:

0+ <.*>:
[ 	]*[a-f0-9]+:	f2 e8 0e 00 00 00    	bnd callq 14 <foo>
[ 	]*[a-f0-9]+:	f2 ff 10             	bnd callq \*\(%rax\)
[ 	]*[a-f0-9]+:	f2 74 08             	bnd je 14 <foo>
[ 	]*[a-f0-9]+:	f2 eb 05             	bnd jmp 14 <foo>
[ 	]*[a-f0-9]+:	f2 ff 23             	bnd jmpq \*\(%rbx\)
[ 	]*[a-f0-9]+:	f2 c3                	bnd retq 

0+14 <foo>:
[ 	]*[a-f0-9]+:	f2 c3                	bnd retq 
[ 	]*[a-f0-9]+:	f2 c3                	bnd retq 
[ 	]*[a-f0-9]+:	f2 e8 f6 ff ff ff    	bnd callq 14 <foo>
[ 	]*[a-f0-9]+:	48 01 c3             	add    %rax,%rbx
[ 	]*[a-f0-9]+:	e2 f1                	loop   14 <foo>
#pass
