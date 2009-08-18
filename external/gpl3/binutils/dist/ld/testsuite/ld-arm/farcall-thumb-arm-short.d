.*:     file format .*

Disassembly of section .text:

00001000 <__bar_from_thumb>:
    1000:	4778      	bx	pc
    1002:	46c0      	nop			\(mov r8, r8\)
    1004:	ea000402 	b	2014 <bar>
00001008 <_start>:
    1008:	f7ff fffa 	bl	1000 <__bar_from_thumb>
Disassembly of section .foo:

00002014 <bar>:
    2014:	e12fff1e 	bx	lr
