#as: -J
#objdump: -dwr
#name: x86-64 branch 2

.*: +file format .*

Disassembly of section .text:

0+ <bar-0x4>:
[ 	]*[a-f0-9]+:	66 e9 00 00          	jmpw   4 <bar>	2: R_X86_64_PC16	foo-0x2

0+4 <bar>:
[ 	]*[a-f0-9]+:	89 c3                	mov    %eax,%ebx
[ 	]*[a-f0-9]+:	66 e8 00 00          	callw  a <bar\+0x6>	8: R_X86_64_PC16	foo-0x2
#pass
