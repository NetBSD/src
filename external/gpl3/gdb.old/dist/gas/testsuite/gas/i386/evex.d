#objdump: -dw -Msuffix
#name: i386 EVX insns

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
 +[a-f0-9]+:	62 f1 d6 38 2a f0    	vcvtsi2ssl %eax,\{rd-sae\},%xmm5,%xmm6
 +[a-f0-9]+:	62 f1 d7 38 2a f0    	vcvtsi2sdl %eax,\(bad\),%xmm5,%xmm6
 +[a-f0-9]+:	62 f1 d6 08 7b f0    	vcvtusi2ssl %eax,%xmm5,%xmm6
 +[a-f0-9]+:	62 f1 d7 08 7b f0    	vcvtusi2sdl %eax,%xmm5,%xmm6
 +[a-f0-9]+:	62 f1 d6 38 7b f0    	vcvtusi2ssl %eax,\{rd-sae\},%xmm5,%xmm6
 +[a-f0-9]+:	62 f1 d7 38 7b f0    	vcvtusi2sdl %eax,\(bad\),%xmm5,%xmm6
#pass
