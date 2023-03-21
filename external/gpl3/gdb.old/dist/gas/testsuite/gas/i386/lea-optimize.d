#as: -O -q
#objdump: -dw
#name: i386 LEA-like segment overrride dropping
#source: lea.s

.*: +file format .*

Disassembly of section .text:

0+ <start>:
[ 	]*[0-9a-f]+:[ 	]+8d 00[ 	]+lea[ 	]+\(%eax\),%eax
[ 	]*[0-9a-f]+:[ 	]+8d 00[ 	]+lea[ 	]+\(%eax\),%eax
#pass
