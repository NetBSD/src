#as: -march=generic32
#objdump: -dw
#name: i386 arch 4

.*:     file format .*

Disassembly of section .text:

0+ <.text>:
[ 	]*[a-f0-9]+:	0f b9                	ud1    
[ 	]*[a-f0-9]+:	0f 0b                	ud2    
[ 	]*[a-f0-9]+:	0f 0b                	ud2    
[ 	]*[a-f0-9]+:	0f b9                	ud1    
#pass
