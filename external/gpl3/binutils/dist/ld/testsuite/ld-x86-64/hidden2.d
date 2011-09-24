#as: --64
#ld: -shared -melf_x86_64
#objdump: -drw

.*: +file format .*


Disassembly of section .text:

[a-f0-9]+ <bar>:
[ 	]*[a-f0-9]+:	e8 33 fe ff ff       	callq  0 .*
[ 	]*[a-f0-9]+:	c3                   	retq   
#pass
