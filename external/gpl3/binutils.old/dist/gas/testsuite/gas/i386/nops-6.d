#objdump: -drw
#name: i386 nops 6

.*: +file format .*

Disassembly of section .text:

0+ <i386>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%esi\),%esi
[ 	]*[a-f0-9]+:	8d bc 27 00 00 00 00 	lea    0x0\(%edi,%eiz,1\),%edi

0+10 <i386_nop>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	66 0f 1f 44 00 00    	nopw   0x0\(%eax,%eax,1\)
[ 	]*[a-f0-9]+:	0f 1f 80 00 00 00 00 	nopl   0x0\(%eax\)
#pass
