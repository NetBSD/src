#as:
#objdump: -dw -Mintel
#name: x86_64 ENQCMD[S] insns (Intel disassembly)
#source: x86-64-enqcmd.s

.*: +file format .*


Disassembly of section \.text:

0+ <_start>:
[ 	]*[a-f0-9]+:[ 	]*f2 0f 38 f8 01[ 	]*enqcmd rax,\[rcx\]
[ 	]*[a-f0-9]+:[ 	]*67 f2 0f 38 f8 01[ 	]*enqcmd eax,\[ecx\]
[ 	]*[a-f0-9]+:[ 	]*f3 0f 38 f8 01[ 	]*enqcmds rax,\[rcx\]
[ 	]*[a-f0-9]+:[ 	]*67 f3 0f 38 f8 01[ 	]*enqcmds eax,\[ecx\]
[ 	]*[a-f0-9]+:[ 	]*f2 0f 38 f8 01[ 	]*enqcmd rax,\[rcx\]
[ 	]*[a-f0-9]+:[ 	]*67 f2 0f 38 f8 01[ 	]*enqcmd eax,\[ecx\]
[ 	]*[a-f0-9]+:[ 	]*f3 0f 38 f8 01[ 	]*enqcmds rax,\[rcx\]
[ 	]*[a-f0-9]+:[ 	]*67 f3 0f 38 f8 01[ 	]*enqcmds eax,\[ecx\]
#pass
