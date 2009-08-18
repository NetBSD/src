#as: -mtune=i686
#source: nops-3.s
#objdump: -drw
#name: i386 -mtune=i686 nops 3

.*: +file format .*

Disassembly of section .text:

0+ <nop>:
[ 	]*[a-f0-9]+:	90                   	nop    
[ 	]*[a-f0-9]+:	90                   	nop    
[ 	]*[a-f0-9]+:	66 66 66 66 66 66 2e 0f 1f 84 00 00 00 00 00 	nopw   %cs:0x0\(%eax,%eax,1\)
[ 	]*[a-f0-9]+:	66 66 66 66 66 66 2e 0f 1f 84 00 00 00 00 00 	nopw   %cs:0x0\(%eax,%eax,1\)
[ 	]*[a-f0-9]+:	89 c3                	mov    %eax,%ebx
[ 	]*[a-f0-9]+:	66 66 66 66 66 2e 0f 1f 84 00 00 00 00 00 	nopw   %cs:0x0\(%eax,%eax,1\)
#pass
