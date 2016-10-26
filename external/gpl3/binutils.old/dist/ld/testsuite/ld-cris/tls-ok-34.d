#source: tls-gottprelm.s --defsym r=8189
#as: --no-underscore --em=criself --pic
#ld: --shared -m crislinux
#objdump: -s -j .got -R

# Check that a R_CRIS_16_DTPREL just below the theoretical limit
# works.  Verify that the first and last R_CRIS_16_GOT_TPREL entries
# are ok, in a DSO.  Beware, the order here is quite random,
# supposedly depending on symbol hashes.

.*:     file format elf32-cris

DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE 
000b3870 R_CRIS_32_TPREL   x2814
#...
000b485c R_CRIS_32_TPREL   x8188
#...
000ba564 R_CRIS_32_TPREL   x0
#...
000bb860 R_CRIS_32_TPREL   x1345

Contents of section .got:
 b3864 ec370b00 00000000 00000000 00000000  .*
 b3874 00000000 00000000 00000000 00000000  .*
#...
 bb844 00000000 00000000 00000000 00000000  .*
 bb854 00000000 00000000 00000000 00000000  .*
