#as: --64
#ld: -shared -melf_x86_64
#objdump: -drw

.*: +file format .*


Disassembly of section .text:

0+[a-f0-9]+ <foo>:
[ 	]*[a-f0-9]+:	c3                   	retq   

0+[a-f0-9]+ <bar>:
[ 	]*[a-f0-9]+:	e8 fa ff ff ff       	callq  [a-f0-9]+ <foo>
[ 	]*[a-f0-9]+:	c3                   	retq   
#pass
