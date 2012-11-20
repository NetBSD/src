#source: expdyn1.s
#as: --no-underscore --em=criself
#ld: -m crislinux -export-dynamic tmpdir/libdso-1.so
#objdump: -T

.*:     file format elf32-cris

# Exporting dynamic symbols means objects as well as functions.

DYNAMIC SYMBOL TABLE:
#...
00080... g    DF .text	00000002 expfn
00082... g    DO .data	00000000 expobj
#pass
