#source: expdref1.s
#as: --pic --no-underscore --em=criself -I$srcdir/$subdir
#ld: --shared -m crislinux
#ld_after_inputfiles: tmpdir/libdso-1b.so
#objdump: -s -T

# A DSO linked to another DSO that has two versioned symbols, to which
# this DSO refers with two relocs each, a GOT and a PLT reference.
# There was a bug such that GOT markups were lost, resulting in wrong
# offsets into the GOT, with a tell-tale sign being one or more
# R_CRIS_NONE relocs.  There should be two GOT relocs here, one each
# for the symbols.

.*:     file format elf32-cris

DYNAMIC SYMBOL TABLE:
#...
0+[ 	]+DF \*UND\*	0+[ 	]+TST2[	 ]+expobj
0+[ 	]+DF \*UND\*	0+[ 	]+TST2[	 ]+expfn
#...
Contents of section .rela.dyn:
 01a4 cc220000 0a030000 00000000 d0220000  .*
 01b4 0a040000 00000000                    .*
Contents of section .plt:
 01bc fce17e7e 0401307a 08013009 00000000  .*
 01cc 00000000 6f0d0c00 00003009 3f7e0000  .*
 01dc 00002ffe ecffffff 6f0d1000 00003009  .*
 01ec 3f7e0000 00002ffe ecffffff           .*
Contents of section .text:
 01f8 6fae0c00 00006fae ccffffff 6fae1000  .*
 0208 00006fae d4ffffff                    .*
#...
Contents of section .got:
 22c0 10220000 00000000 00000000 00000000  .*
 22d0 00000000                             .*
