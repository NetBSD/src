#source: expdyn1.s
#source: expdref1.s --pic
#as: --no-underscore --em=criself
#ld: -m crislinux --export-dynamic tmpdir/libdso-1.so --hash-style=sysv
#objdump: -s -j .got

# Like expdyn2.d, but testing that the .got contents is correct.  There
# needs to be a .got due to the GOT relocs, but the entry is constant.

.*:     file format elf32-cris
Contents of section \.got:
 82244 dc210800 00000000 00000000 bf010800  .*
 82254 58220800                             .*
