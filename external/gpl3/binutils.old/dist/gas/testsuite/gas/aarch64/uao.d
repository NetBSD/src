#objdump: -dr
#as: -march=armv8.2-a

.*:     file format .*

Disassembly of section \.text:

0000000000000000 <.*>:
   [0-9a-f]:+	d500417f 	msr	uao, #0x1
   [0-9a-f]:+	d500407f 	msr	uao, #0x0
   [0-9a-f]:+	d5184280 	msr	uao, x0
   [0-9a-f]:+	d5384281 	mrs	x1, uao
