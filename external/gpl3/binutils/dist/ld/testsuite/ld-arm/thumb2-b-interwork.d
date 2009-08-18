
.*thumb2-b-interwork:     file format elf32-.*arm

Disassembly of section .text:

00008000 <_start>:
    8000:	f000 b802 	b.w	8008 <__bar_from_thumb>

00008004 <bar>:
    8004:	e12fff1e 	bx	lr
Disassembly of section .glue_7t:

00008008 <__bar_from_thumb>:
    8008:	4778      	bx	pc
    800a:	46c0      	nop			\(mov r8, r8\)

0000800c <__bar_change_to_arm>:
    800c:	eafffffc 	b	8004 <bar>

