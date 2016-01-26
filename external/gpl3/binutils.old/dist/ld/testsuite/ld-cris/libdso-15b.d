#source: expdref2.s
#as: --pic --no-underscore --em=criself
#ld: --shared -m crislinux
#ld_after_inputfiles: tmpdir/libdso-15.so
#objdump: -s -T

# A DSO that refers to two symbols in another DSO with
# GOT/PLT-generating relocs, using weak aliases where the strong
# definition is also in the other DSO.
# There was a bug causing GOT markups to be during symbol handling,
# with a newly added assertion failure and a reloc turned R_CRIS_NONE.
# The dynamic __expobj2 entry is unused and seems spurious, or there
# should at least be a similar __expfn2 reference

.*:     file format elf32-cris

DYNAMIC SYMBOL TABLE:
#...
0+[	 ]+DO \*UND\*[	 ]+0+  TST3[	 ]+__expobj2
#...
0+[	 ]+DO \*UND\*[	 ]+0+  TST3[	 ]+expobj2
0+[	 ]+DF \*UND\*[	 ]+0+  TST3[	 ]+expfn2
#...
Contents of section .rela.dyn:
 01d4 fc220000 0a040000 00000000 00230000  .*
 01e4 0a050000 00000000                    .*
Contents of section .plt:
 01ec fce17e7e 0401307a 08013009 00000000  .*
 01fc 00000000 6f0d0c00 00003009 3f7e0000  .*
 020c 00002ffe ecffffff 6f0d1000 00003009  .*
 021c 3f7e0000 00002ffe ecffffff           .*
Contents of section .text:
 0228 6fae0c00 00006fae ccffffff 6fae1000  .*
 0238 00006fae d4ffffff                    .*
#...
Contents of section .got:
 22f0 40220000 00000000 00000000 00000000  .*
 2300 00000000                             .*
