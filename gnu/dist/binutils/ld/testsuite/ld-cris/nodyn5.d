#source: expdyn1.s
#source: expdref1.s --pic
#source: comref1.s --pic
#as: --no-underscore --em=criself
#ld: -m crislinux
#objdump: -s -j .got

# Like nodyn4.d, but checking .got contents.

.*:     file format elf32-cris
Contents of section \.got:
 820c0 00000000 00000000 00000000 dc200800  .*
 820d0 76000800 d8200800                    .*
