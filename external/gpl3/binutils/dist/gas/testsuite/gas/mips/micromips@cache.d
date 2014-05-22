#objdump: -dr --prefix-addresses --show-raw-insn
#name: MIPS CACHE instruction
#as: -32 --defsym micromips=1
#source: cache.s

# Check MIPS CACHE instruction assembly (microMIPS).

.*: +file format .*mips.*

Disassembly of section \.text:
[0-9a-f]+ <[^>]*> 20a2 67ff 	cache	0x5,2047\(v0\)
[0-9a-f]+ <[^>]*> 20a3 6800 	cache	0x5,-2048\(v1\)
[0-9a-f]+ <[^>]*> 3020 1000 	li	at,4096
[0-9a-f]+ <[^>]*> 0081 0950 	addu	at,at,a0
[0-9a-f]+ <[^>]*> 20a1 6800 	cache	0x5,-2048\(at\)
[0-9a-f]+ <[^>]*> 3020 f000 	li	at,-4096
[0-9a-f]+ <[^>]*> 00a1 0950 	addu	at,at,a1
[0-9a-f]+ <[^>]*> 20a1 67ff 	cache	0x5,2047\(at\)
[0-9a-f]+ <[^>]*> 5020 8000 	li	at,0x8000
[0-9a-f]+ <[^>]*> 00c1 0950 	addu	at,at,a2
[0-9a-f]+ <[^>]*> 20a1 6fff 	cache	0x5,-1\(at\)
[0-9a-f]+ <[^>]*> 3020 8000 	li	at,-32768
[0-9a-f]+ <[^>]*> 00e1 0950 	addu	at,at,a3
[0-9a-f]+ <[^>]*> 20a1 6000 	cache	0x5,0\(at\)
[0-9a-f]+ <[^>]*> 5020 8000 	li	at,0x8000
[0-9a-f]+ <[^>]*> 0101 0950 	addu	at,at,t0
[0-9a-f]+ <[^>]*> 20a1 6000 	cache	0x5,0\(at\)
[0-9a-f]+ <[^>]*> 3020 8000 	li	at,-32768
[0-9a-f]+ <[^>]*> 0121 0950 	addu	at,at,t1
[0-9a-f]+ <[^>]*> 20a1 6fff 	cache	0x5,-1\(at\)
[0-9a-f]+ <[^>]*> 5020 9000 	li	at,0x9000
[0-9a-f]+ <[^>]*> 0141 0950 	addu	at,at,t2
[0-9a-f]+ <[^>]*> 20a1 6000 	cache	0x5,0\(at\)
[0-9a-f]+ <[^>]*> 41a1 ffff 	lui	at,0xffff
[0-9a-f]+ <[^>]*> 5021 7000 	ori	at,at,0x7000
[0-9a-f]+ <[^>]*> 0161 0950 	addu	at,at,t3
[0-9a-f]+ <[^>]*> 20a1 6fff 	cache	0x5,-1\(at\)
	\.\.\.
