#objdump: -drw
#name: i386 nops 5

.*: +file format .*

Disassembly of section .text:

0+ <i386>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%esi\),%esi
[ 	]*[a-f0-9]+:	8d bc 27 00 00 00 00 	lea    0x0\(%edi,%eiz,1\),%edi

0+10 <i486>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%esi\),%esi
[ 	]*[a-f0-9]+:	8d bc 27 00 00 00 00 	lea    0x0\(%edi,%eiz,1\),%edi

0+20 <i586>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%esi\),%esi
[ 	]*[a-f0-9]+:	8d bc 27 00 00 00 00 	lea    0x0\(%edi,%eiz,1\),%edi

0+30 <i686>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%esi\),%esi
[ 	]*[a-f0-9]+:	8d bc 27 00 00 00 00 	lea    0x0\(%edi,%eiz,1\),%edi

0+40 <pentium4>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	0f 1f 00             	nopl   \(%eax\)
[ 	]*[a-f0-9]+:	66 2e 0f 1f 84 00 00 00 00 00 	nopw   %cs:0x0\(%eax,%eax,1\)

0+50 <nocona>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	0f 1f 00             	nopl   \(%eax\)
[ 	]*[a-f0-9]+:	66 2e 0f 1f 84 00 00 00 00 00 	nopw   %cs:0x0\(%eax,%eax,1\)

0+60 <core>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	0f 1f 00             	nopl   \(%eax\)
[ 	]*[a-f0-9]+:	66 2e 0f 1f 84 00 00 00 00 00 	nopw   %cs:0x0\(%eax,%eax,1\)

0+70 <core2>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	0f 1f 00             	nopl   \(%eax\)
[ 	]*[a-f0-9]+:	66 2e 0f 1f 84 00 00 00 00 00 	nopw   %cs:0x0\(%eax,%eax,1\)

0+80 <k6>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%esi\),%esi
[ 	]*[a-f0-9]+:	8d bc 27 00 00 00 00 	lea    0x0\(%edi,%eiz,1\),%edi

0+90 <athlon>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	0f 1f 00             	nopl   \(%eax\)
[ 	]*[a-f0-9]+:	66 2e 0f 1f 84 00 00 00 00 00 	nopw   %cs:0x0\(%eax,%eax,1\)

0+a0 <k8>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	0f 1f 00             	nopl   \(%eax\)
[ 	]*[a-f0-9]+:	66 2e 0f 1f 84 00 00 00 00 00 	nopw   %cs:0x0\(%eax,%eax,1\)

0+b0 <generic32>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%esi\),%esi
[ 	]*[a-f0-9]+:	8d bc 27 00 00 00 00 	lea    0x0\(%edi,%eiz,1\),%edi

0+c0 <generic64>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	0f 1f 00             	nopl   \(%eax\)
[ 	]*[a-f0-9]+:	66 2e 0f 1f 84 00 00 00 00 00 	nopw   %cs:0x0\(%eax,%eax,1\)

0+d0 <amdfam10>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	0f 1f 00             	nopl   \(%eax\)
[ 	]*[a-f0-9]+:	66 2e 0f 1f 84 00 00 00 00 00 	nopw   %cs:0x0\(%eax,%eax,1\)
#pass
