#as:
#objdump: -dw -Mintel
#name: i386 MOVDIR[I,64B] insns (Intel disassembly)
#source: movdir.s

.*: +file format .*


Disassembly of section \.text:

00000000 <_start>:
[ 	]*[a-f0-9]+:[ 	]*0f 38 f9 01[ 	]*movdiri DWORD PTR \[ecx\],eax
[ 	]*[a-f0-9]+:[ 	]*66 0f 38 f8 01[ 	]*movdir64b eax,\[ecx\]
[ 	]*[a-f0-9]+:[ 	]*67 66 0f 38 f8 04[ 	]*movdir64b ax,\[si\]
[ 	]*[a-f0-9]+:[ 	]*0f 38 f9 01[ 	]*movdiri DWORD PTR \[ecx\],eax
[ 	]*[a-f0-9]+:[ 	]*0f 38 f9 01[ 	]*movdiri DWORD PTR \[ecx\],eax
[ 	]*[a-f0-9]+:[ 	]*66 0f 38 f8 01[ 	]*movdir64b eax,\[ecx\]
[ 	]*[a-f0-9]+:[ 	]*67 66 0f 38 f8 04[ 	]*movdir64b ax,\[si\]
[ 	]*[a-f0-9]+:[ 	]*67 0f 38 f9 01[ 	]*movdiri DWORD PTR \[bx\+di\],eax
[ 	]*[a-f0-9]+:[ 	]*67 66 0f 38 f8 01[ 	]*movdir64b ax,\[bx\+di\]
[ 	]*[a-f0-9]+:[ 	]*66 0f 38 f8 04 67[ 	]*movdir64b eax,\[edi\+eiz\*2\]
[ 	]*[a-f0-9]+:[ 	]*0f 38 f9 01[ 	]*movdiri DWORD PTR \[ecx\],eax
[ 	]*[a-f0-9]+:[ 	]*67 0f 38 f9 01[ 	]*movdiri DWORD PTR \[bx\+di\],eax
[ 	]*[a-f0-9]+:[ 	]*67 66 0f 38 f8 01[ 	]*movdir64b ax,\[bx\+di\]
[ 	]*[a-f0-9]+:[ 	]*66 0f 38 f8 04 90[ 	]*movdir64b eax,\[eax\+edx\*4\]
#pass
