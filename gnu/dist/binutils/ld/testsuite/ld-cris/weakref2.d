#source: gotrel2.s
#as: --pic --no-underscore --em=criself
#ld: -m crislinux tmpdir/libdso-1.so
#objdump: -s -j .got

# Like weakref1.d, but check contents of .got.

.*:     file format elf32-cris
Contents of section \.got:
 82208 a0210800 00000000 00000000 00000000  .*
