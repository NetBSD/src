#source: tls-dtprelm.s --defsym r=32767
#as: --no-underscore --em=criself --pic
#ld: --shared -m crislinux
#objdump: -s -j .got -j .text -j .tdata -R

# Check that a R_CRIS_16_DTPREL just below the theoretical limit
# works, in a DSO.

.*:     file format elf32-cris

DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE 
0000a220 R_CRIS_DTPMOD     \*ABS\*

Contents of section \.text:
 01a0 5faeff7f                             .*
Contents of section \.tdata:
 21a4 2a2a2a2a 2a2a2a2a 2a2a2a2a 2a2a2a2a  .*
#...
 a194 2a2a2a2a 2a2a2a2a 2a2a2a2a 2a2a2a2a  .*
Contents of section \.got:
 a214 a4a10000 00000000 00000000 00000000  .*
 a224 00000000                             .*
