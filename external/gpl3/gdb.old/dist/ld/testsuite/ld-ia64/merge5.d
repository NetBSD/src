#source: merge5.s
#as: -x
#ld: -shared --hash-style=sysv
#objdump: -d

#...
0+1f0 <.text>:
[ 	]*[a-f0-9]+:	0b 60 80 02 00 24 	\[MMI\]       addl r12=32,r1;;
[ 	]*[a-f0-9]+:	c0 40 05 00 48 00 	            addl r12=40,r1
[ 	]*[a-f0-9]+:	00 00 04 00       	            nop.i 0x0;;
[ 	]*[a-f0-9]+:	0b 60 a0 02 00 24 	\[MMI\]       addl r12=40,r1;;
[ 	]*[a-f0-9]+:	c0 c0 05 00 48 00 	            addl r12=56,r1
[ 	]*[a-f0-9]+:	00 00 04 00       	            nop.i 0x0;;
[ 	]*[a-f0-9]+:	01 60 60 02 00 24 	\[MII\]       addl r12=24,r1
[ 	]*[a-f0-9]+:	00 00 00 02 00 00 	            nop.i 0x0
[ 	]*[a-f0-9]+:	00 00 04 00       	            nop.i 0x0;;
