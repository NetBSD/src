#objdump: -dw
#name: i386 xsave

.*:     file format .*

Disassembly of section .text:

0+ <_start>:
[ 	]*[a-f0-9]+:	0f ae 2b             	xrstor \(%ebx\)
[ 	]*[a-f0-9]+:	0f ae 23             	xsave  \(%ebx\)
[ 	]*[a-f0-9]+:	0f ae 33             	xsaveopt \(%ebx\)
[ 	]*[a-f0-9]+:	0f 01 d0             	xgetbv 
[ 	]*[a-f0-9]+:	0f 01 d1             	xsetbv 
[ 	]*[a-f0-9]+:	0f ae 29             	xrstor \(%ecx\)
[ 	]*[a-f0-9]+:	0f ae 21             	xsave  \(%ecx\)
[ 	]*[a-f0-9]+:	0f ae 31             	xsaveopt \(%ecx\)
#pass
