#as:
#objdump: -dw
#name: i386 MOVDIR[I,64B] insns
#source: movdir.s

.*: +file format .*


Disassembly of section \.text:

00000000 <_start>:
[ 	]*[a-f0-9]+:[ 	]*0f 38 f9 01[ 	]*movdiri %eax,\(%ecx\)
[ 	]*[a-f0-9]+:[ 	]*66 0f 38 f8 01[ 	]*movdir64b \(%ecx\),%eax
[ 	]*[a-f0-9]+:[ 	]*67 66 0f 38 f8 04[ 	]*movdir64b \(%si\),%ax
[ 	]*[a-f0-9]+:[ 	]*0f 38 f9 01[ 	]*movdiri %eax,\(%ecx\)
[ 	]*[a-f0-9]+:[ 	]*0f 38 f9 01[ 	]*movdiri %eax,\(%ecx\)
[ 	]*[a-f0-9]+:[ 	]*66 0f 38 f8 01[ 	]*movdir64b \(%ecx\),%eax
[ 	]*[a-f0-9]+:[ 	]*67 66 0f 38 f8 04[ 	]*movdir64b \(%si\),%ax
[ 	]*[a-f0-9]+:[ 	]*67 0f 38 f9 01[ 	]*movdiri %eax,\(%bx,%di\)
[ 	]*[a-f0-9]+:[ 	]*67 66 0f 38 f8 01[ 	]*movdir64b \(%bx,%di\),%ax
[ 	]*[a-f0-9]+:[ 	]*66 0f 38 f8 04 67[ 	]*movdir64b \(%edi,%eiz,2\),%eax
[ 	]*[a-f0-9]+:[ 	]*0f 38 f9 01[ 	]*movdiri %eax,\(%ecx\)
[ 	]*[a-f0-9]+:[ 	]*67 0f 38 f9 01[ 	]*movdiri %eax,\(%bx,%di\)
[ 	]*[a-f0-9]+:[ 	]*67 66 0f 38 f8 01[ 	]*movdir64b \(%bx,%di\),%ax
[ 	]*[a-f0-9]+:[ 	]*66 0f 38 f8 04 90[ 	]*movdir64b \(%eax,%edx,4\),%eax
#pass
