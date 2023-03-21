#objdump: -r -w
#as: --generate-missing-build-notes=no
# The HPPA maps R_PARISC_PCREL64 onto BFD_RELOC_NONE.
#skip: ft32-*-* hppa*-*-*

#...
0+ .*(NONE|NULL|UNUSED0) +\*ABS\*
