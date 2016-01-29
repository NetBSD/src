#as:
#objdump: -dw
#name: x86_64 PCOMMIT insns
#source: x86-64-pcommit.s

.*: +file format .*


Disassembly of section \.text:

0+ <_start>:
[ 	]*[a-f0-9]+:[ 	]*66 0f ae f8[ 	]*pcommit 
[ 	]*[a-f0-9]+:[ 	]*66 0f ae f8[ 	]*pcommit 
#pass
