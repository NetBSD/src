#as: --EB
#source: reloc-insn32.s
#objdump: -d
#ld: -Tdata=0xdabeef -EB
#name: reloc INSN32 BE

.*: +file format .*bpfbe

Disassembly of section .text:

[0-9a-f]+ <main>:
 *[0-9a-f]+:	b7 10 00 00 00 da be f3[ 	]*mov %r1,0xdabef3
 *[0-9a-f]+:	16 10 00 02 00 da be f3[ 	]*jeq32 %r1,0xdabef3,2
 *[0-9a-f]+:	38 00 00 00 00 da be ff[ 	]*ldabsdw 0xdabeff
 *[0-9a-f]+:	95 00 00 00 00 00 00 00[ 	]*exit

[0-9a-f]+ <baz>:
 *[0-9a-f]+:	07 10 00 00 00 da be ef[ 	]*add %r1,0xdabeef
 *[0-9a-f]+:	62 20 00 08 00 da be f7[ 	]*stw \[%r2\+8\],0xdabef7
