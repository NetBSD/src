#source: pic-gc-73.s
#source: expdyn1.s
#as: --pic --no-underscore --em=criself -I$srcdir/$subdir
#ld: --shared -m crislinux --gc-sections
#objdump: -s -t -R -p -T

# Left-over code coverage case from pic-gc-72; local symbol.

.*:     file format elf32-cris
#...
DYNAMIC RELOCATION RECORDS \(none\)

Contents of section .hash:
#...
Contents of section .dynsym:
#...
Contents of section .dynstr:
#...
Contents of section .text:
 016e 0f050f05                             .*
Contents of section .dynamic:
#...
Contents of section .got:
 21cc 74210000 00000000 00000000           .*
Contents of section .data:
 21d8 00000000                             .*
