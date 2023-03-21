#PROG: objcopy
#source: remove-relocs-01.s
#objcopy: --remove-relocations=.data.relocs.01
#readelf: -r
#notarget: "mips64*-*-openbsd*"

Relocation section '\.rela?\.data\.relocs\.02' at offset 0x[0-9a-f]+ contains 3 entries:
.*
.*
.*
.*

Relocation section '\.rela?\.data\.relocs\.03' at offset 0x[0-9a-f]+ contains 3 entries:
.*
.*
.*
.*
