#objdump: -dw
#name: i386 prefix

.*: +file format .*

Disassembly of section .text:

0+000 <foo>:
   0:	9b 26 67 d9 3c[ 	]+fstcw[ 	]+%es:\(%si\)
   5:	9b df e0 [ 	]*fstsw  %ax
   8:	9b df e0 [ 	]*fstsw  %ax
   b:	9b 67 df e0 [ 	]*addr16 fstsw %ax
   f:	36 67 66 f3 a7 [ 	]*repz cmpsw %es:\(%di\),%ss:\(%si\)
#pass
