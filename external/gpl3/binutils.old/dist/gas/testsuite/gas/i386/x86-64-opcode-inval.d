#as: --32
#objdump: -dw -Mx86-64
#name: 64bit illegal opcodes

.*: +file format .*

Disassembly of section .text:

0+ <aaa>:
[ 	]*[a-f0-9]+:	37                   	\(bad\)  

0+1 <aad0>:
[ 	]*[a-f0-9]+:	d5                   	\(bad\)  
[ 	]*[a-f0-9]+:	0a d5                	or     %ch,%dl

0+3 <aad1>:
[ 	]*[a-f0-9]+:	d5                   	\(bad\)  
[ 	]*[a-f0-9]+:	02 d4                	add    %ah,%dl

0+5 <aam0>:
[ 	]*[a-f0-9]+:	d4                   	\(bad\)  
[ 	]*[a-f0-9]+:	0a d4                	or     %ah,%dl

0+7 <aam1>:
[ 	]*[a-f0-9]+:	d4                   	\(bad\)  
[ 	]*[a-f0-9]+:	02 3f                	add    \(%rdi\),%bh

0+9 <aas>:
[ 	]*[a-f0-9]+:	3f                   	\(bad\)  

0+a <bound>:
[ 	]*[a-f0-9]+:	62                   	\(bad\)  
[ 	]*[a-f0-9]+:	10 27                	adc    %ah,\(%rdi\)

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
