#source: ../x86-64-opcode-inval.s
#as: --32
#objdump: -dw -Mx86-64 -Mintel
#name: x86-64 (ILP32) illegal opcodes (Intel mode)

.*: +file format .*

Disassembly of section .text:

0+ <aaa>:
[ 	]*[a-f0-9]+:	37                   	\(bad\)  

0+1 <aad0>:
[ 	]*[a-f0-9]+:	d5                   	\(bad\)  
[ 	]*[a-f0-9]+:	0a d5                	or     dl,ch

0+3 <aad1>:
[ 	]*[a-f0-9]+:	d5                   	\(bad\)  
[ 	]*[a-f0-9]+:	02 d4                	add    dl,ah

0+5 <aam0>:
[ 	]*[a-f0-9]+:	d4                   	\(bad\)  
[ 	]*[a-f0-9]+:	0a d4                	or     dl,ah

0+7 <aam1>:
[ 	]*[a-f0-9]+:	d4                   	\(bad\)  
[ 	]*[a-f0-9]+:	02 3f                	add    bh,BYTE PTR \[rdi\]

0+9 <aas>:
[ 	]*[a-f0-9]+:	3f                   	\(bad\)  

0+a <bound>:
[ 	]*[a-f0-9]+:	62                   	\(bad\)  
[ 	]*[a-f0-9]+:	10 27                	adc    BYTE PTR \[rdi\],ah

0+c <daa>:
[ 	]*[a-f0-9]+:	27                   	\(bad\)  

0+d <das>:
[ 	]*[a-f0-9]+:	2f                   	\(bad\)  

0+e <into>:
[ 	]*[a-f0-9]+:	ce                   	\(bad\)  

0+f <pusha>:
[ 	]*[a-f0-9]+:	60                   	\(bad\)  

0+10 <popa>:
[ 	]*[a-f0-9]+:	61                   	\(bad\)  
#pass
