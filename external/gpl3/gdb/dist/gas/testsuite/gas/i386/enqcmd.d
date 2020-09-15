#as:
#objdump: -dw
#name: i386 ENQCMD[S] insns
#source: enqcmd.s

.*: +file format .*


Disassembly of section \.text:

00000000 <_start>:
[ 	]*[a-f0-9]+:[ 	]*f2 0f 38 f8 01[ 	]*enqcmd \(%ecx\),%eax
[ 	]*[a-f0-9]+:[ 	]*67 f2 0f 38 f8 04[ 	]*enqcmd \(%si\),%ax
[ 	]*[a-f0-9]+:[ 	]*f3 0f 38 f8 01[ 	]*enqcmds \(%ecx\),%eax
[ 	]*[a-f0-9]+:[ 	]*67 f3 0f 38 f8 04[ 	]*enqcmds \(%si\),%ax
[ 	]*[a-f0-9]+:[ 	]*f2 0f 38 f8 01[ 	]*enqcmd \(%ecx\),%eax
[ 	]*[a-f0-9]+:[ 	]*67 f2 0f 38 f8 04[ 	]*enqcmd \(%si\),%ax
[ 	]*[a-f0-9]+:[ 	]*f3 0f 38 f8 01[ 	]*enqcmds \(%ecx\),%eax
[ 	]*[a-f0-9]+:[ 	]*67 f3 0f 38 f8 04[ 	]*enqcmds \(%si\),%ax
#pass
