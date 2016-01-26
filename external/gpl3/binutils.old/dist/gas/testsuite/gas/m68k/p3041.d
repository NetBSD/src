#name: PR 3041
#objdump: -tdr

.*:     file format .*

SYMBOL TABLE:
00000036  w      \.text 0000 00 0f mytext
0000003e  w      \.data 0000 00 10 mydata
0000004a  w      \.bss  0000 00 11 mybss

Disassembly of section \.text:

00000000 <.*>:
   0:	41f9 0000 0000 	lea 0 <.*>,%a0
			2: 32	mytext
   6:	41f9 0000 0002 	lea 2 <.*>,%a0
			8: 32	mytext
   c:	41f9 ffff fffc 	lea fffffffc <.*>,%a0
			e: 32	mytext
  12:	41f9 0000 0000 	lea 0 <.*>,%a0
			14: 32	mydata
  18:	41f9 0000 0003 	lea 3 <.*>,%a0
			1a: 32	mydata
  1e:	41f9 ffff ffff 	lea ffffffff <.*>,%a0
			20: 32	mydata
  24:	41f9 0000 0000 	lea 0 <.*>,%a0
			26: 32	mybss
  2a:	41f9 0000 0001 	lea 1 <.*>,%a0
			2c: 32	mybss
  30:	41f9 ffff fffe 	lea fffffffe <.*>,%a0
			32: 32	mybss

00000036 <mytext>:
  36:	4e71           	nop
  38:	4e71           	nop
  3a:	4e71           	nop
