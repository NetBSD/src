#as:
#objdump: -dw -Mintel
#name: x86_64 MOVDIR[I,64B] insns (Intel disassembly)
#source: x86-64-movdir.s

.*: +file format .*


Disassembly of section \.text:

0+ <_start>:
[ 	]*[a-f0-9]+:[ 	]*48 0f 38 f9 01[ 	]*movdiri QWORD PTR \[rcx\],rax
[ 	]*[a-f0-9]+:[ 	]*66 0f 38 f8 01[ 	]*movdir64b rax,\[rcx\]
[ 	]*[a-f0-9]+:[ 	]*67 66 0f 38 f8 01[ 	]*movdir64b eax,\[ecx]
[ 	]*[a-f0-9]+:[ 	]*0f 38 f9 01[ 	]*movdiri DWORD PTR \[rcx\],eax
[ 	]*[a-f0-9]+:[ 	]*48 0f 38 f9 01[ 	]*movdiri QWORD PTR \[rcx\],rax
[ 	]*[a-f0-9]+:[ 	]*0f 38 f9 01[ 	]*movdiri DWORD PTR \[rcx\],eax
[ 	]*[a-f0-9]+:[ 	]*48 0f 38 f9 01[ 	]*movdiri QWORD PTR \[rcx\],rax
[ 	]*[a-f0-9]+:[ 	]*66 0f 38 f8 01[ 	]*movdir64b rax,\[rcx\]
[ 	]*[a-f0-9]+:[ 	]*67 66 0f 38 f8 01[ 	]*movdir64b eax,\[ecx\]
#pass
