#source: ifunc-8a.s
#source: ifunc-8b.s
#ld: 
#readelf: -r --wide
#target: aarch64*-*-*

Relocation section '.rela.plt' at .*
[ ]+Offset[ ]+Info[ ]+Type[ ]+.*
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_AARCH64_IRELATIVE[ ]+[0-9a-f]*
