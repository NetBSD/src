#source: tls-small-ld.s
#ld: -T relocs.ld -shared
#readelf: -Wr

Relocation section '\.rela\.dyn' at offset 0x10070 contains 2 entries:
    Offset             Info             Type               Symbol's Value  Symbol's Name \+ Addend
0000000000000000  0000000000000000 R_AARCH64_NONE                            0
0000000000020008  0000000200000404 R_AARCH64_TLS_DTPMOD64 0000000000000000 dummy \+ 0

