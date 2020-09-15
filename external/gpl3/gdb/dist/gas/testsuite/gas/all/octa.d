#objdump: -s -j .data
#name: octa bignum

.*: +file format .*

Contents of section .data:
 [^ ]* (ffff3344 55667788 99aabbcc ddeeffff|ffffeedd ccbbaa99 88776655 4433ffff) .*
 [^ ]* (00003444 55667788 99aabbcc ddeeffff|ffffeedd ccbbaa99 88776655 44340000) .*
