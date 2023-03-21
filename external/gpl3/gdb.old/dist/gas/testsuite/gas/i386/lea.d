#objdump: -dw
#name: i386 LEA-like warnings
#warning_output: lea.e

.*: +file format .*

Disassembly of section .text:

0+ <start>:
[ 	]*[0-9a-f]+:[ 	]+36 8d 00[ 	]+lea[ 	]+%ss:\(%eax\),%eax
[ 	]*[0-9a-f]+:[ 	]+36 8d 00[ 	]+lea[ 	]+%ss:\(%eax\),%eax
#pass
