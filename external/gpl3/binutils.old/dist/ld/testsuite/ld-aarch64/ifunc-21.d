#source: ifunc-21.s
#ld: -shared -z nocombreloc
#objdump: -d -s -j .got.plt -j .text
#target: aarch64*-*-*

# Ensure the .got.plt slot used is correct

.*:     file format elf64-(little|big)aarch64

Contents of section .text:
 [0-9a-f]+ .*
Contents of section .got.plt:
 [0-9a-f]+ 0+ 0+ 0+ 0+  .*
 (103b8|10408) 0+ 0+ [0-9a-f]+ [0-9a-f]+  .*

Disassembly of section .text:

.* <ifunc>:
 .*:	d65f03c0 	ret

.* <bar>:
 .*:	90000080 	adrp	x0, 10000 <.*>
 .*:	.* 	ldr	x0, \[x0,#(960|1040)\]
 .*:	d65f03c0 	ret

#pass