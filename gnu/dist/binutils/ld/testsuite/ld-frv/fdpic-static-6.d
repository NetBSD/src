#name: FRV uClinux PIC relocs to weak undefined symbols, static linking
#source: fdpic6.s
#objdump: -D
#ld: -static
#warning: different segment

.*:     file format elf.*frv.*

Disassembly of section \.text:

00010094 <F6>:
   10094:	fe 3f bf db 	call 0 <_gp-0xf8d8>
   10098:	80 40 f0 0c 	addi gr15,12,gr0
   1009c:	80 fc 00 24 	setlos 0x24,gr0
   100a0:	80 f4 00 20 	setlo 0x20,gr0
   100a4:	80 f8 00 00 	sethi hi\(0x0\),gr0
   100a8:	80 40 f0 10 	addi gr15,16,gr0
   100ac:	80 fc 00 18 	setlos 0x18,gr0
   100b0:	80 f4 00 1c 	setlo 0x1c,gr0
   100b4:	80 f8 00 00 	sethi hi\(0x0\),gr0
   100b8:	80 40 ff f8 	addi gr15,-8,gr0
   100bc:	80 fc ff f0 	setlos 0xf*fffffff0,gr0
   100c0:	80 f4 ff e8 	setlo 0xffe8,gr0
   100c4:	80 f8 ff ff 	sethi 0xffff,gr0
   100c8:	80 f4 be e0 	setlo 0xbee0,gr0
   100cc:	80 f8 ff fe 	sethi 0xfffe,gr0
   100d0:	80 f4 00 14 	setlo 0x14,gr0
   100d4:	80 f8 00 00 	sethi hi\(0x0\),gr0
Disassembly of section \.rofixup:

000100d8 <__ROFIXUP_LIST__>:
   100d8:	00 01 41 20 	sub\.p gr20,gr32,gr0
Disassembly of section \.data:

000140dc <D6>:
	\.\.\.
Disassembly of section \.got:

000140e8 <_GLOBAL_OFFSET_TABLE_-0x38>:
	\.\.\.

00014120 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
