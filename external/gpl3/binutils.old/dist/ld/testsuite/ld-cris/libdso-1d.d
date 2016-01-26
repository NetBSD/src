#source: expdref1.s
#source: expdyn1w.s
#as: --pic --no-underscore --em=criself -I$srcdir/$subdir
#ld: --shared -m crislinux
#ld_after_inputfiles: tmpdir/libdso-1b.so
#objdump: -s -T

# A DSO linked to another DSO that has two versioned symbols, to which
# this DSO refers with two relocs each, a GOT and a PLT reference.
# This DSO has weak definitions of those symbols.

.*:     file format elf32-cris

DYNAMIC SYMBOL TABLE:
#...
0+1f2  w   DF .text	0+2 expfn
0+22a0  w   DO .data	0+4 expobj
#...
Contents of section .rela.dyn:
 0184 98220000 0a020000 00000000 9c220000  .*
 0194 0a030000 00000000                    .*
Contents of section .plt:
 019c fce17e7e 0401307a 08013009 00000000  .*
 01ac 00000000 6f0d0c00 00003009 3f7e0000  .*
 01bc 00002ffe ecffffff 6f0d1000 00003009  .*
 01cc 3f7e0000 00002ffe ecffffff           .*
Contents of section .text:
 01d8 6fae1000 00006fae e0ffffff 6fae0c00  .*
 01e8 00006fae c0ffffff 0f050f05           .*
Contents of section .dynamic:
#...
Contents of section .got:
 228c f4210000 00000000 00000000 00000000  .*
 229c 00000000                             .*
Contents of section .data:
 22a0 00000000                             .*
