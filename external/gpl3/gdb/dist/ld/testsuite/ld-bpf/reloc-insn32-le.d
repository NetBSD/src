#as: --EL
#source: reloc-insn32.s
#objdump: -d
#ld: -Tdata=0xdabeef -EL
#name: reloc INSN32 LE

.*: +file format .*bpfle

Disassembly of section .text:

[0-9a-f]+ <main>:
 *[0-9a-f]+:	b7 01 00 00 f3 be da 00[ 	]*mov %r1,0xdabef3
 *[0-9a-f]+:	16 01 02 00 f3 be da 00[ 	]*jeq32 %r1,0xdabef3,2
 *[0-9a-f]+:	38 00 00 00 ff be da 00[ 	]*ldabsdw 0xdabeff
 *[0-9a-f]+:	95 00 00 00 00 00 00 00[ 	]*exit

[0-9a-f]+ <baz>:
 *[0-9a-f]+:	07 01 00 00 ef be da 00[ 	]*add %r1,0xdabeef
 *[0-9a-f]+:	62 02 08 00 f7 be da 00[ 	]*stw \[%r2\+8\],0xdabef7
