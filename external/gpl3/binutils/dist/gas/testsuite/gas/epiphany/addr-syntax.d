#as:
#objdump: -dr
#name: addr-syntax

.*.o:     file format elf32-epiphany


Disassembly of section \.text:

00000000 \<\.text\>:
   0:	2bcc 01ff 	ldr.l r1,\[r2,-0x7ff\]
   4:	4c4c 0301 	ldr.l r2,\[r3\],-0x8
   8:	107c 2201 	strd.l r8,\[r4\],\+0x8
   c:	506c 2400 	ldrd sl,\[ip\]
  10:	587c 2400 	strd sl,\[lr\]
