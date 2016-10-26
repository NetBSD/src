#source: ifunc-6.s
#ld: -shared -z nocombreloc
#readelf: -r --wide
#target: aarch64*-*-*

Relocation section '.rela.got' at .*
[ ]+Offset[ ]+Info[ ]+Type[ ]+.*
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_AARCH64_GLOB_DAT[ ]+foo\(\)[ ]+foo \+ 0
#...
Relocation section '.rela.plt' at .*
[ ]+Offset[ ]+Info[ ]+Type[ ]+.*
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_AARCH64_IRELATIVE[ ]+[0-9a-f]*
