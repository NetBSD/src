#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0000000000000000 <.*>:
   0:	d50340df 	msr	daifset, #0x0
   4:	d50341df 	msr	daifset, #0x1
   8:	d5034fdf 	msr	daifset, #0xf
   c:	d50340ff 	msr	daifclr, #0x0
  10:	d50341ff 	msr	daifclr, #0x1
  14:	d5034fff 	msr	daifclr, #0xf
  18:	d51b4220 	msr	daif, x0
  1c:	d53b4220 	mrs	x0, daif
