#as: -march=i386 -mtune=core2
#source: nops-2.s
#objdump: -drw
#name: i386 -march=i386 -mtune=core2 nops 2

.*: +file format .*

Disassembly of section .text:

0+ <nop>:
 +[a-f0-9]+:	0f be f0             	movsbl %al,%esi
 +[a-f0-9]+:	8d b4 26 00 00 00 00 	lea    0x0\(%esi,%eiz,1\),%esi
 +[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%esi\),%esi

0+10 <nop15>:
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	8d b4 26 00 00 00 00 	lea    0x0\(%esi,%eiz,1\),%esi
 +[a-f0-9]+:	8d b4 26 00 00 00 00 	lea    0x0\(%esi,%eiz,1\),%esi
 +[a-f0-9]+:	90                   	nop

0+20 <nop14>:
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	8d b4 26 00 00 00 00 	lea    0x0\(%esi,%eiz,1\),%esi
 +[a-f0-9]+:	8d b4 26 00 00 00 00 	lea    0x0\(%esi,%eiz,1\),%esi

0+30 <nop13>:
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	8d b4 26 00 00 00 00 	lea    0x0\(%esi,%eiz,1\),%esi
 +[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%esi\),%esi

0+40 <nop12>:
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	8d b4 26 00 00 00 00 	lea    0x0\(%esi,%eiz,1\),%esi
 +[a-f0-9]+:	8d 74 26 00          	lea    0x0\(%esi,%eiz,1\),%esi
 +[a-f0-9]+:	90                   	nop

0+50 <nop11>:
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	8d b4 26 00 00 00 00 	lea    0x0\(%esi,%eiz,1\),%esi
 +[a-f0-9]+:	8d 74 26 00          	lea    0x0\(%esi,%eiz,1\),%esi

0+60 <nop10>:
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	8d b4 26 00 00 00 00 	lea    0x0\(%esi,%eiz,1\),%esi
 +[a-f0-9]+:	8d 76 00             	lea    0x0\(%esi\),%esi

0+70 <nop9>:
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	8d b4 26 00 00 00 00 	lea    0x0\(%esi,%eiz,1\),%esi
 +[a-f0-9]+:	66 90                	xchg   %ax,%ax

0+80 <nop8>:
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	8d b4 26 00 00 00 00 	lea    0x0\(%esi,%eiz,1\),%esi
 +[a-f0-9]+:	90                   	nop

0+90 <nop7>:
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	8d b4 26 00 00 00 00 	lea    0x0\(%esi,%eiz,1\),%esi

0+a0 <nop6>:
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%esi\),%esi

0+b0 <nop5>:
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	8d 74 26 00          	lea    0x0\(%esi,%eiz,1\),%esi
 +[a-f0-9]+:	90                   	nop

0+c0 <nop4>:
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	8d 74 26 00          	lea    0x0\(%esi,%eiz,1\),%esi

0+d0 <nop3>:
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	8d 76 00             	lea    0x0\(%esi\),%esi

0+e0 <nop2>:
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	90                   	nop
 +[a-f0-9]+:	66 90                	xchg   %ax,%ax
#pass
