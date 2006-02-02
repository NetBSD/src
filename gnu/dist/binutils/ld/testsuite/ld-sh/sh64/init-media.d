#source: init.s
#as: --abi=32 --isa=SHmedia
#ld: -shared -mshelf32
#readelf: -d
#target: sh64-*-elf

# Make sure that the lsb of DT_INIT and DT_FINI entries is set
# when _init and _fini are SHmedia code.

Dynamic section at offset 0x238 contains 8 entries:
  Tag        Type                         Name/Value
 0x0000000c \(INIT\)                       0x217
 0x0000000d \(FINI\)                       0x227
 0x00000004 \(HASH\)                       0x94
 0x00000005 \(STRTAB\)                     0x1c0
 0x00000006 \(SYMTAB\)                     0xe0
 0x0000000a \(STRSZ\)                      85 \(bytes\)
 0x0000000b \(SYMENT\)                     16 \(bytes\)
 0x00000000 \(NULL\)                       0x0
