#objdump: -dw
#name: i386 arch 8

.*:     file format .*

Disassembly of section .text:

0+ <.text>:
[ 	]*[a-f0-9]+:	f3 0f b8 d9          	popcnt %ecx,%ebx
[ 	]*[a-f0-9]+:	0f 7a 12 ca          	frczss %xmm2,%xmm1
#pass
