#source: dso-2.s
#source: dsofnf2.s
#source: gotrel1.s
#as: --pic --no-underscore --em=criself
#ld: -m crislinux tmpdir/libdso-1.so
#objdump: -sR

# Make sure we don't merge a PLT-specific entry
# (R_CRIS_JUMP_SLOT) with a non-PLT-GOT-specific entry
# (R_CRIS_GLOB_DAT) in an executable, since they may have
# different contents there.  (If we merge them in a DSO it's ok:
# we make a round-trip to the PLT in the executable if it's
# referenced there, but that's still perceived as better than
# having an unnecessary PLT, dynamic reloc and lookup in the
# DSO.)  In the executable, the GOT contents for the non-PLT
# reloc should be constant.

.*:     file format elf32-cris

DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE 
000822b4 R_CRIS_JUMP_SLOT  dsofn

Contents of section .*
#...
Contents of section \.rela\.plt:
 801ac b4220800 0b050000 00000000           .*
Contents of section \.plt:
 801b8 fce17e7e 7f0dac22 0800307a 7f0db022  .*
 801c8 08003009 7f0db422 08003009 3f7e0000  .*
 801d8 00002ffe d8ffffff                    .*
Contents of section \.text:
 801e0 5f1d0c00 30096f1d 0c000000 30090000  .*
 801f0 6f0d1000 0000611a 6f2ecc01 08000000  .*
 80200 6f3e58df ffff0000                    .*
Contents of section \.dynamic:
 82220 01000000 01000000 04000000 e4000800  .*
 82230 05000000 70010800 06000000 10010800  .*
 82240 0a000000 3b000000 0b000000 10000000  .*
 82250 15000000 00000000 03000000 a8220800  .*
 82260 02000000 0c000000 14000000 07000000  .*
 82270 17000000 ac010800 00000000 00000000  .*
 82280 00000000 00000000 00000000 00000000  .*
 82290 00000000 00000000 00000000 00000000  .*
 822a0 00000000 00000000                    .*
Contents of section \.got:
 822a8 20220800 00000000 00000000 d4010800  .*
 822b8 cc010800                             .*
