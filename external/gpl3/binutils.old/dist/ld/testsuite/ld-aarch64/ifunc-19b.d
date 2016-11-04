#source: ifunc-19b.s
#source: ifunc-19a.s
#ld: -shared -z nocombreloc
#readelf: -r --wide
#target: aarch64*-*-*

Relocation section '.rela.ifunc' at .*
[ ]+Offset[ ]+Info[ ]+Type[ ]+.*
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_AARCH64_IRELATIVE[ ]+[0-9a-f]*

Relocation section '.rela.plt' at .*
[ ]+Offset[ ]+Info[ ]+Type[ ]+.*
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_AARCH64_IRELATIVE[ ]+[0-9a-f]*
