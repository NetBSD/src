#source: ifunc-5-local.s
#ld: -r
#readelf: -r --wide
#target: aarch64*-*-*

Relocation section '.rela.text' at .*
[ ]+Offset[ ]+Info[ ]+Type[ ]+.*
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_AARCH64_CALL26[ ]+foo\(\)[ ]+foo \+ 0
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_AARCH64_ADR_GOT_PAGE[ ]+foo\(\)[ ]+foo \+ 0
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_AARCH64_LD64_GOT_LO1[ ]+foo\(\)[ ]+foo \+ 0
