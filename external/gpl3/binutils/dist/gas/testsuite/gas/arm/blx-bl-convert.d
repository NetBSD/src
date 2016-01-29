#name: blx->bl convert under no -march/cpu
#error-output: blx-bl-convert.l
#objdump: -d

.*:     file format .*

Disassembly of section \.text:

00000000 <entry>:
   0:	f000 f800 	bl	4 <label>

00000004 <label>:
   4:	4770      	bx	lr
	\.\.\.

00000008 <label2>:
   8:	ebffffff 	bl	c <label3>

0000000c <label3>:
   c:	e12fff1e 	bx	lr

