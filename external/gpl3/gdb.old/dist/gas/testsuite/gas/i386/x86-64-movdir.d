#as:
#objdump: -dw
#name: x86_64 MOVDIR[I,64B] insns
#source: x86-64-movdir.s

.*: +file format .*


Disassembly of section \.text:

0+ <_start>:
[ 	]*[a-f0-9]+:[ 	]*48 0f 38 f9 01[ 	]*movdiri %rax,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*66 0f 38 f8 01[ 	]*movdir64b \(%rcx\),%rax
[ 	]*[a-f0-9]+:[ 	]*67 66 0f 38 f8 01[ 	]*movdir64b \(%ecx\),%eax
[ 	]*[a-f0-9]+:[ 	]*0f 38 f9 01[ 	]*movdiri %eax,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*48 0f 38 f9 01[ 	]*movdiri %rax,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*0f 38 f9 01[ 	]*movdiri %eax,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*48 0f 38 f9 01[ 	]*movdiri %rax,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*66 0f 38 f8 01[ 	]*movdir64b \(%rcx\),%rax
[ 	]*[a-f0-9]+:[ 	]*67 66 0f 38 f8 01[ 	]*movdir64b \(%ecx\),%eax
#pass
