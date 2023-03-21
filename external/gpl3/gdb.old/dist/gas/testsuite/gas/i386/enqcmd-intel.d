#as:
#objdump: -dw -Mintel
#name: i386 ENQCMD[S] insns (Intel disassembly)
#source: enqcmd.s

.*: +file format .*


Disassembly of section \.text:

00000000 <_start>:
[ 	]*[a-f0-9]+:[ 	]*f2 0f 38 f8 01[ 	]*enqcmd eax,\[ecx\]
[ 	]*[a-f0-9]+:[ 	]*67 f2 0f 38 f8 04[ 	]*enqcmd ax,\[si\]
[ 	]*[a-f0-9]+:[ 	]*f3 0f 38 f8 01[ 	]*enqcmds eax,\[ecx\]
[ 	]*[a-f0-9]+:[ 	]*67 f3 0f 38 f8 04[ 	]*enqcmds ax,\[si\]
[ 	]*[a-f0-9]+:[ 	]*f2 0f 38 f8 01[ 	]*enqcmd eax,\[ecx\]
[ 	]*[a-f0-9]+:[ 	]*67 f2 0f 38 f8 04[ 	]*enqcmd ax,\[si\]
[ 	]*[a-f0-9]+:[ 	]*f3 0f 38 f8 01[ 	]*enqcmds eax,\[ecx\]
[ 	]*[a-f0-9]+:[ 	]*67 f3 0f 38 f8 04[ 	]*enqcmds ax,\[si\]
#pass
