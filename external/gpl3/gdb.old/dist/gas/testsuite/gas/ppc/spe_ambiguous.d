#as: -a32 -mbig -mvle
#objdump: -d -Mspe
#name: Validate SPE instructions

.*: +file format elf.*-powerpc.*

Disassembly of section .text:

00000000 <.text>:
   0:	10 01 12 04 	evsubfw r0,r1,r2
   4:	10 01 12 04 	evsubw r0,r2,r1
   8:	10 1f 12 06 	evsubifw r0,31,r2
   c:	10 1f 12 06 	evsubiw r0,r2,31
  10:	10 01 12 18 	evnor   r0,r1,r2
  14:	10 01 0a 18 	evnot   r0,r1
