#name: FRV TLS relocs, static linking with relaxation
#source: tls-1.s
#objdump: -D -j .text -j .got -j .plt
#ld: -static tmpdir/tls-1-dep.o --relax

.*:     file format elf.*frv.*

Disassembly of section \.text:

000100b4 <_start>:
   100b4:	92 fc f8 10 	setlos 0xf*fffff810,gr9
   100b8:	00 88 00 00 	nop\.p
   100bc:	80 88 00 00 	nop
   100c0:	92 fc f8 10 	setlos 0xf*fffff810,gr9
   100c4:	80 88 00 00 	nop
   100c8:	12 fc f8 10 	setlos\.p 0xf*fffff810,gr9
   100cc:	80 88 00 00 	nop
   100d0:	80 88 00 00 	nop
   100d4:	92 fc f8 20 	setlos 0xf*fffff820,gr9
   100d8:	00 88 00 00 	nop\.p
   100dc:	80 88 00 00 	nop
   100e0:	92 fc f8 20 	setlos 0xf*fffff820,gr9
   100e4:	80 88 00 00 	nop
   100e8:	12 fc f8 20 	setlos\.p 0xf*fffff820,gr9
   100ec:	80 88 00 00 	nop
   100f0:	80 88 00 00 	nop
   100f4:	92 fc f8 30 	setlos 0xf*fffff830,gr9
   100f8:	00 88 00 00 	nop\.p
   100fc:	80 88 00 00 	nop
   10100:	92 fc f8 30 	setlos 0xf*fffff830,gr9
   10104:	80 88 00 00 	nop
   10108:	12 fc f8 30 	setlos\.p 0xf*fffff830,gr9
   1010c:	80 88 00 00 	nop
   10110:	80 88 00 00 	nop
   10114:	92 fc 00 00 	setlos lo\(0x0\),gr9
   10118:	00 88 00 00 	nop\.p
   1011c:	80 88 00 00 	nop
   10120:	92 fc 00 00 	setlos lo\(0x0\),gr9
   10124:	80 88 00 00 	nop
   10128:	12 fc 00 00 	setlos\.p lo\(0x0\),gr9
   1012c:	80 88 00 00 	nop
   10130:	80 88 00 00 	nop
   10134:	00 88 00 00 	nop\.p
   10138:	90 fc f8 30 	setlos 0xf*fffff830,gr8
   1013c:	00 88 00 00 	nop\.p
   10140:	92 fc f8 20 	setlos 0xf*fffff820,gr9
   10144:	92 fc f8 10 	setlos 0xf*fffff810,gr9
   10148:	92 fc f8 20 	setlos 0xf*fffff820,gr9
   1014c:	92 fc f8 30 	setlos 0xf*fffff830,gr9
   10150:	92 fc 00 00 	setlos lo\(0x0\),gr9
   10154:	00 88 00 00 	nop\.p
   10158:	80 88 00 00 	nop
   1015c:	92 fc f8 10 	setlos 0xf*fffff810,gr9
   10160:	00 88 00 00 	nop\.p
   10164:	80 88 00 00 	nop
   10168:	92 fc f8 20 	setlos 0xf*fffff820,gr9
   1016c:	00 88 00 00 	nop\.p
   10170:	80 88 00 00 	nop
   10174:	92 fc f8 30 	setlos 0xf*fffff830,gr9
   10178:	00 88 00 00 	nop\.p
   1017c:	80 88 00 00 	nop
   10180:	92 fc 00 00 	setlos lo\(0x0\),gr9
