#source: expdyn2.s
#as: --pic --no-underscore --em=criself -I$srcdir/$subdir
#ld: --shared -m crislinux --version-script $srcdir/$subdir/expalltst3
#objdump: -s -T

# A DSO that has two versioned symbols, each with a weak alias.
# Each symbol is versioned.

.*:     file format elf32-cris

DYNAMIC SYMBOL TABLE:
#...
0+2288 g[ 	]+DO .data[	 ]+0+4  TST3[ 	]+__expobj2
0+20a g[ 	]+DF .text[	 ]+0+2  TST3[ 	]+__expfn2
0+20a  w[ 	]+DF .text[	 ]+0+2  TST3[ 	]+expfn2
0+2288  w[ 	]+DO .data[	 ]+0+4  TST3[ 	]+expobj2
#...
Contents of section .text:
 0208 0f050f05                             .*
#...
Contents of section .got:
 227c 0c220000 00000000 00000000           .*
Contents of section .data:
 2288 00000000                             .*
