#as:
#objdump: -dw
#name: x86_64 ENQCMD[S] insns
#source: x86-64-enqcmd.s

.*: +file format .*


Disassembly of section \.text:

0+ <_start>:
[ 	]*[a-f0-9]+:[ 	]*f2 0f 38 f8 01[ 	]*enqcmd \(%rcx\),%rax
[ 	]*[a-f0-9]+:[ 	]*67 f2 0f 38 f8 01[ 	]*enqcmd \(%ecx\),%eax
[ 	]*[a-f0-9]+:[ 	]*f3 0f 38 f8 01[ 	]*enqcmds \(%rcx\),%rax
[ 	]*[a-f0-9]+:[ 	]*67 f3 0f 38 f8 01[ 	]*enqcmds \(%ecx\),%eax
[ 	]*[a-f0-9]+:[ 	]*f2 0f 38 f8 01[ 	]*enqcmd \(%rcx\),%rax
[ 	]*[a-f0-9]+:[ 	]*67 f2 0f 38 f8 01[ 	]*enqcmd \(%ecx\),%eax
[ 	]*[a-f0-9]+:[ 	]*f3 0f 38 f8 01[ 	]*enqcmds \(%rcx\),%rax
[ 	]*[a-f0-9]+:[ 	]*67 f3 0f 38 f8 01[ 	]*enqcmds \(%ecx\),%eax
#pass
