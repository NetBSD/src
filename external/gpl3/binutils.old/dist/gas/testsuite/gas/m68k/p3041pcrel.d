#name: PR 3041 pcrel
#as: -m68000
#objdump: -tdr

.*:     file format .*

SYMBOL TABLE:
00000024  w      \.text 0000 00 0f mytext
0000002e  w      \.data 0000 00 10 mydata
0000003a  w      \.bss  0000 00 11 mybss

Disassembly of section \.text:

00000000 <.*>:
   0:	41fa fffe      	lea %pc@\(0 <.*>\),%a0
			2: DISP16	mytext
   4:	41fa fffc      	lea %pc@\(2 <.*>\),%a0
			6: DISP16	mytext
   8:	41fa fff2      	lea %pc@\(fffffffc <.*>\),%a0
			a: DISP16	mytext
   c:	41fa fff2      	lea %pc@\(0 <.*>\),%a0
			e: DISP16	mydata
  10:	41fa fff1      	lea %pc@\(3 <.*>\),%a0
			12: DISP16	mydata
  14:	41fa ffe9      	lea %pc@\(ffffffff <.*>\),%a0
			16: DISP16	mydata
  18:	41fa ffe6      	lea %pc@\(0 <.*>\),%a0
			1a: DISP16	mybss
  1c:	41fa ffe3      	lea %pc@\(1 <.*>\),%a0
			1e: DISP16	mybss
  20:	41fa ffdc      	lea %pc@\(fffffffe <.*>\),%a0
			22: DISP16	mybss

00000024 <mytext>:
  24:	4e71           	nop
  26:	4e71           	nop
  28:	4e71           	nop
  2a:	4e71           	nop
