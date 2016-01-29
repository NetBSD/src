#as:
#objdump: -dw
#name: i386 PCOMMIT insns
#source: pcommit.s

.*: +file format .*


Disassembly of section \.text:

00000000 <_start>:
[ 	]*[a-f0-9]+:[ 	]*66 0f ae f8[ 	]*pcommit 
[ 	]*[a-f0-9]+:[ 	]*66 0f ae f8[ 	]*pcommit 
#pass
