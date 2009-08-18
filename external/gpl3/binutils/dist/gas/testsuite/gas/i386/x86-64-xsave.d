#objdump: -dw
#name: x86-64 xsave

.*: +file format .*

Disassembly of section .text:

0+ <_start>:
[ 	]*[a-f0-9]+:	41 0f ae 29          	xrstor \(%r9\)
[ 	]*[a-f0-9]+:	41 0f ae 21          	xsave  \(%r9\)
[ 	]*[a-f0-9]+:	0f 01 d0             	xgetbv 
[ 	]*[a-f0-9]+:	0f 01 d1             	xsetbv 
[ 	]*[a-f0-9]+:	0f ae 29             	xrstor \(%rcx\)
[ 	]*[a-f0-9]+:	0f ae 21             	xsave  \(%rcx\)
#pass
