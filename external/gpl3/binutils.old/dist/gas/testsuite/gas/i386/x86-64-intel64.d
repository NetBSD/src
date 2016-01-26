#objdump: -dw
#name: x86-64 Intel64

.*: +file format .*

Disassembly of section .text:
0+ <_start>:
[ 	]*[a-f0-9]+:	0f 05                	syscall 
[ 	]*[a-f0-9]+:	0f 07                	sysret 
#pass
