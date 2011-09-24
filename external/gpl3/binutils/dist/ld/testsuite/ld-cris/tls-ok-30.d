#source: tls-gdgotrelm.s --defsym r=8191
#as: --no-underscore --em=criself --pic
#ld: --shared -m crislinux
#objdump: -s -j .got -R

# Verify that the first and last R_CRIS_16_GOT_GD entries are ok just
# below the limit, in a DSO.  Beware, the order here is quite random,
# supposedly depending on symbol hashes.

.*:     file format elf32-cris

DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE 
000b3910 R_CRIS_DTP        x2814
#...
000b8350 R_CRIS_DTP        x8190
#...
000c1308 R_CRIS_DTP        x0
#...
000c3900 R_CRIS_DTP        x1345

Contents of section .got:
 b3904 94380b00 00000000 00000000 00000000  .*
 b3914 00000000 00000000 00000000 00000000  .*
#...
 c38e4 00000000 00000000 00000000 00000000  .*
 c38f4 00000000 00000000 00000000 00000000  .*
 c3904 00000000                             .*
#PASS
