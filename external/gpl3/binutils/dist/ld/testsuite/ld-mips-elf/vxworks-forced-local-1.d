#as: -mips2 -mvxworks-pic
#source: vxworks-forced-local-1.s
#ld: -shared -Tvxworks1.ld --version-script vxworks-forced-local-1.ver
#readelf: --relocs

Relocation section '\.rela\.dyn' .*
.*
0008140c  00000002 R_MIPS_32 *00080810
00081410  00000002 R_MIPS_32 *00080814
00081414  00000002 R_MIPS_32 *00080818
00081418  00000302 R_MIPS_32 *00000000 *bar \+ 0
#pass
