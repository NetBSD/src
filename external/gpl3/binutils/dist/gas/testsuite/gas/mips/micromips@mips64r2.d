#objdump: -dr --prefix-addresses --show-raw-insn -M reg-names=numeric
#name: MIPS MIPS64r2 instructions
#source: mips64r2.s

# Check MIPS64r2 instruction assembly (microMIPS).

.*: +file format .*mips.*

Disassembly of section \.text:
[0-9a-f]+ <[^>]*> 5843 002c 	dext	\$2,\$3,0x0,0x1
[0-9a-f]+ <[^>]*> 5843 f82c 	dext	\$2,\$3,0x0,0x20
[0-9a-f]+ <[^>]*> 5843 0024 	dextm	\$2,\$3,0x0,0x21
[0-9a-f]+ <[^>]*> 5843 f824 	dextm	\$2,\$3,0x0,0x40
[0-9a-f]+ <[^>]*> 5843 07ec 	dext	\$2,\$3,0x1f,0x1
[0-9a-f]+ <[^>]*> 5843 ffec 	dext	\$2,\$3,0x1f,0x20
[0-9a-f]+ <[^>]*> 5843 07e4 	dextm	\$2,\$3,0x1f,0x21
[0-9a-f]+ <[^>]*> 5843 0014 	dextu	\$2,\$3,0x20,0x1
[0-9a-f]+ <[^>]*> 5843 f814 	dextu	\$2,\$3,0x20,0x20
[0-9a-f]+ <[^>]*> 5843 07d4 	dextu	\$2,\$3,0x3f,0x1
[0-9a-f]+ <[^>]*> 5843 5aa4 	dextm	\$2,\$3,0xa,0x2c
[0-9a-f]+ <[^>]*> 5843 5a94 	dextu	\$2,\$3,0x2a,0xc
[0-9a-f]+ <[^>]*> 5843 000c 	dins	\$2,\$3,0x0,0x1
[0-9a-f]+ <[^>]*> 5843 f80c 	dins	\$2,\$3,0x0,0x20
[0-9a-f]+ <[^>]*> 5843 0004 	dinsm	\$2,\$3,0x0,0x21
[0-9a-f]+ <[^>]*> 5843 f804 	dinsm	\$2,\$3,0x0,0x40
[0-9a-f]+ <[^>]*> 5843 ffcc 	dins	\$2,\$3,0x1f,0x1
[0-9a-f]+ <[^>]*> 5843 07c4 	dinsm	\$2,\$3,0x1f,0x2
[0-9a-f]+ <[^>]*> 5843 ffc4 	dinsm	\$2,\$3,0x1f,0x21
[0-9a-f]+ <[^>]*> 5843 0034 	dinsu	\$2,\$3,0x20,0x1
[0-9a-f]+ <[^>]*> 5843 f834 	dinsu	\$2,\$3,0x20,0x20
[0-9a-f]+ <[^>]*> 5843 fff4 	dinsu	\$2,\$3,0x3f,0x1
[0-9a-f]+ <[^>]*> 5843 aa84 	dinsm	\$2,\$3,0xa,0x2c
[0-9a-f]+ <[^>]*> 5843 aab4 	dinsu	\$2,\$3,0x2a,0xc
[0-9a-f]+ <[^>]*> 5b2a e0c8 	dror32	\$25,\$10,0x1c
[0-9a-f]+ <[^>]*> 5b2a 20c0 	dror	\$25,\$10,0x4
[0-9a-f]+ <[^>]*> 5b2a e0c0 	dror	\$25,\$10,0x1c
[0-9a-f]+ <[^>]*> 5b2a 20c8 	dror32	\$25,\$10,0x4
[0-9a-f]+ <[^>]*> 5880 c9d0 	dnegu	\$25,\$4
[0-9a-f]+ <[^>]*> 5959 c8d0 	drorv	\$25,\$10,\$25
[0-9a-f]+ <[^>]*> 5944 c8d0 	drorv	\$25,\$10,\$4
[0-9a-f]+ <[^>]*> 5b2a 20c8 	dror32	\$25,\$10,0x4
[0-9a-f]+ <[^>]*> 5944 c8d0 	drorv	\$25,\$10,\$4
[0-9a-f]+ <[^>]*> 58e7 7b3c 	dsbh	\$7,\$7
[0-9a-f]+ <[^>]*> 590a 7b3c 	dsbh	\$8,\$10
[0-9a-f]+ <[^>]*> 58e7 fb3c 	dshd	\$7,\$7
[0-9a-f]+ <[^>]*> 590a fb3c 	dshd	\$8,\$10
	\.\.\.
