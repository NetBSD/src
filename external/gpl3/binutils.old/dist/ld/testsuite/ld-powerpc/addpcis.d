#source: addpcis.s
#as: -a64 -mpower9
#ld: -melf64ppc -Ttext=0x10000000 --defsym ext1=0 --defsym ext2=0x8fff0000
#objdump: -d -Mpower9

.*:     file format .*

Disassembly of section \.text:

0+10000000 <_start>:
    10000000:	(4c 60 f0 04|04 f0 60 4c) 	addpcis r3,-4096
    10000004:	(38 63 ff fc|fc ff 63 38) 	addi    r3,r3,-4
    10000008:	(4c 9f 7f c5|c5 7f 9f 4c) 	addpcis r4,32767
    1000000c:	(38 84 ff f4|f4 ff 84 38) 	addi    r4,r4,-12
    10000010:	(4c a0 00 05|05 00 a0 4c) 	addpcis r5,1
    10000014:	(38 a5 80 00|00 80 a5 38) 	addi    r5,r5,-32768
	\.\.\.

0+10008014 <forw>:
    10008014:	(60 00 00 00|00 00 00 60) 	nop
