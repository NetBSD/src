#source: ifunc-7.s
#ld: -shared
#readelf: -r --wide
#target: aarch64*-*-*

Relocation section '.rela.plt' at .*
[ ]+Offset[ ]+Info[ ]+Type[ ]+.*
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_AARCH64_IRELATIVE[ ]+[0-9a-f]*
