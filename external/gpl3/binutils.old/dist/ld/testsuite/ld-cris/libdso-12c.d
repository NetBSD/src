#source: expdyn1.s
#source: dsov32-1.s
#source: dsov32-2.s
#source: dso-1.s
#as: --pic --no-underscore --march=v32 --em=criself
#ld: --shared -m crislinux --version-script $srcdir/$subdir/hidedsofns2468
#objdump: -s -T

# Like libdso-12b.d, but dsofn is defined and the two called functions
# are forced local using a linker script.  There should just be the
# GOT relocation for expobj in the DSO.

.*:     file format elf32-cris

DYNAMIC SYMBOL TABLE:
#...
0+202 g[ 	]+DF \.text	0+2  Base[ 	]+expfn
0+22bc g[ 	]+DO \.data	0+4  Base[ 	]+expobj
#...
0+204 g[ 	]+DF \.text	0+8  Base[ 	]+dsofn3
#...
Contents of section \.rela\.dyn:
 01f4 b8220000 0a040000 00000000           .*
Contents of section \.text:
 0200 b005b005 bfbe1c00 0000b005 7f0da020  .*
 0210 00005f0d 0c00bfbe f6ffffff b0050000  .*
 0220 b0050000                             .*
Contents of section .dynamic:
 2224 04000000 94000000 05000000 70010000  .*
 2234 06000000 d0000000 0a000000 37000000  .*
 2244 0b000000 10000000 07000000 f4010000  .*
 2254 08000000 0c000000 09000000 0c000000  .*
 2264 fcffff6f bc010000 fdffff6f 02000000  .*
 2274 f0ffff6f a8010000 00000000 00000000  .*
 2284 00000000 00000000 00000000 00000000  .*
 2294 00000000 00000000 00000000 00000000  .*
 22a4 00000000 00000000                    .*
Contents of section \.got:
 22ac 24220000 00000000 00000000 00000000  .*
Contents of section \.data:
 22bc 00000000                             .*
