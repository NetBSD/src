#source: pie.s
#as: -march=from-abi -mabi=64 -EB
#ld: -melf64btsmip -pie
#readelf: -d

Dynamic section at offset 0x208 contains 17 entries:
  Tag * Type * Name/Value
 0x0+00000004 \(HASH\) * 0x368
 0x0+00000005 \(STRTAB\) * 0x498
 0x0+00000006 \(SYMTAB\) * 0x3a8
 0x0+0000000a \(STRSZ\) * 72 \(bytes\)
 0x0+0000000b \(SYMENT\) * 24 \(bytes\)
 0x0+70000035 \(MIPS_RLD_MAP_REL\) * 0x102b8
 0x0+00000015 \(DEBUG\) * 0x0
 0x0+00000003 \(PLTGOT\) * 0x10520
 0x0+70000001 \(MIPS_RLD_VERSION\) * 1
 0x0+70000005 \(MIPS_FLAGS\) * NOTPOT
 0x0+70000006 \(MIPS_BASE_ADDRESS\) * 0x0
 0x0+7000000a \(MIPS_LOCAL_GOTNO\) * 2
 0x0+70000011 \(MIPS_SYMTABNO\) * 10
 0x0+70000012 \(MIPS_UNREFEXTNO\) * 13
 0x0+70000013 \(MIPS_GOTSYM\) * 0xa
 0x0+6ffffffb \(FLAGS_1\) * Flags: PIE
 0x0+00000000 \(NULL\) * 0x0
