#source: init.s
#as: --abi=64
#ld: -shared -mshelf64
#readelf: -d
#target: sh64-*-elf

# Make sure that the lsb of DT_INIT and DT_FINI entries is set
# when _init and _fini are SHmedia code.

Dynamic segment at offset 0x488 contains 8 entries:
  Tag        Type                         Name/Value
 0x000000000000000c \(INIT\)               0x463
 0x000000000000000d \(FINI\)               0x473
 0x0000000000000004 \(HASH\)               0xe8
 0x0000000000000005 \(STRTAB\)             0x3f0
 0x0000000000000006 \(SYMTAB\)             0x198
 0x000000000000000a \(STRSZ\)              114 \(bytes\)
 0x000000000000000b \(SYMENT\)             24 \(bytes\)
 0x0000000000000000 \(NULL\)               0x0
