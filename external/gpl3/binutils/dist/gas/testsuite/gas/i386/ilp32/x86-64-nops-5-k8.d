#source: ../nops-5.s
#as: -march=k8
#objdump: -drw
#name: x86-64 (ILP32) -march=k8 nops 5

.*: +file format .*

Disassembly of section .text:

0+ <i386>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%rsi\),%esi
[ 	]*[a-f0-9]+:	8d bc 27 00 00 00 00 	lea    0x0\(%rdi,%riz,1\),%edi

0+10 <i486>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%rsi\),%esi
[ 	]*[a-f0-9]+:	8d bc 27 00 00 00 00 	lea    0x0\(%rdi,%riz,1\),%edi

0+20 <i586>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%rsi\),%esi
[ 	]*[a-f0-9]+:	8d bc 27 00 00 00 00 	lea    0x0\(%rdi,%riz,1\),%edi

0+30 <i686>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%rsi\),%esi
[ 	]*[a-f0-9]+:	8d bc 27 00 00 00 00 	lea    0x0\(%rdi,%riz,1\),%edi

0+40 <pentium4>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	66 66 66 66 2e 0f 1f 84 00 00 00 00 00 	data32 data32 data32 nopw %cs:0x0\(%rax,%rax,1\)

0+50 <nocona>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	66 66 66 66 2e 0f 1f 84 00 00 00 00 00 	data32 data32 data32 nopw %cs:0x0\(%rax,%rax,1\)

0+60 <core>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	66 66 66 66 2e 0f 1f 84 00 00 00 00 00 	data32 data32 data32 nopw %cs:0x0\(%rax,%rax,1\)

0+70 <core2>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	66 66 66 66 2e 0f 1f 84 00 00 00 00 00 	data32 data32 data32 nopw %cs:0x0\(%rax,%rax,1\)

0+80 <k6>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%rsi\),%esi
[ 	]*[a-f0-9]+:	8d bc 27 00 00 00 00 	lea    0x0\(%rdi,%riz,1\),%edi

0+90 <athlon>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	66 0f 1f 44 00 00    	nopw   0x0\(%rax,%rax,1\)
[ 	]*[a-f0-9]+:	0f 1f 80 00 00 00 00 	nopl   0x0\(%rax\)

0+a0 <k8>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	66 0f 1f 44 00 00    	nopw   0x0\(%rax,%rax,1\)
[ 	]*[a-f0-9]+:	0f 1f 80 00 00 00 00 	nopl   0x0\(%rax\)

0+b0 <generic32>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	8d b6 00 00 00 00    	lea    0x0\(%rsi\),%esi
[ 	]*[a-f0-9]+:	8d bc 27 00 00 00 00 	lea    0x0\(%rdi,%riz,1\),%edi

0+c0 <generic64>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	66 66 66 66 2e 0f 1f 84 00 00 00 00 00 	data32 data32 data32 nopw %cs:0x0\(%rax,%rax,1\)

0+d0 <amdfam10>:
[ 	]*[a-f0-9]+:	0f be f0             	movsbl %al,%esi
[ 	]*[a-f0-9]+:	66 0f 1f 44 00 00    	nopw   0x0\(%rax,%rax,1\)
[ 	]*[a-f0-9]+:	0f 1f 80 00 00 00 00 	nopl   0x0\(%rax\)
#pass
