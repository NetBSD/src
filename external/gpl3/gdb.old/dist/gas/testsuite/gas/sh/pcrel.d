#as: -big
#objdump: -d
#name: PC-relative loads
#warning_output: pcrel.l

.*:     file format .*sh.*

Disassembly of section .text:

00000000 <code>:
   0:	d0 04       	mov\.l	14 <litpool>,r0	! ffffffec
   2:	d1 05       	mov\.l	18 <litpool\+0x4>,r1
   4:	d1 03       	mov\.l	14 <litpool>,r1	! ffffffec
   6:	d1 03       	mov\.l	14 <litpool>,r1	! ffffffec
   8:	c7 02       	mova	14 <litpool>,r0
   a:	61 02       	mov\.l	@r0,r1
   c:	d1 01       	mov\.l	14 <litpool>,r1	! ffffffec
   e:	01 03       	bsrf	r1
  10:	00 09       	nop	
  12:	00 09       	nop	

00000014 <litpool>:
  14:	ff ff       	\.word 0xffff
  16:	ff ec       	\.word 0xffec
