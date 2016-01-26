#source: tls-local-59.s
#as: --pic --no-underscore --em=criself -I$srcdir/$subdir
#ld: -m crislinux --shared
#objdump: -s -t -r -p -R -T

# A DSO with a R_CRIS_32_GOT_GD, a R_CRIS_16_GOT_GD, a
# R_CRIS_32_GOT_TPREL and a R_CRIS_16_GOT_TPREL against the same local
# symbol.  Check that we have proper NPTL/TLS markings and GOT.

.*:     file format elf32-cris

Program Header:
#...
     TLS off .*
         filesz 0x00000080 memsz 0x00000080 flags r--
#...
  FLAGS                0x00000010
#...
DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE 
00002304 R_CRIS_32_TPREL   \*ABS\*
00002308 R_CRIS_DTP        \*ABS\*

Contents of section \.hash:
#...
Contents of section \.text:
 01ec 6fae1000 00006fae 0c000000 5fae1000  .*
 01fc 5fae0c00                             .*
#...
Contents of section \.got:
 22f8 80220+ 0+ 0+ 0+  .*
 2308 0+ 0+                    .*
