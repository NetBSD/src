#name: --extract-symbol test 1 (sections)
#source: extract-symbol-1.s
#ld: -Textract-symbol-1.ld
#objcopy_linked_file: --extract-symbol
#objdump: --headers
#xfail: "hppa*-*-*" "rx-*-*"
# FAILS on the RX because the linker has to set LMA == VMA for the Renesas loader.
#...
Sections:
 *Idx +Name +Size +VMA +LMA .*
 *0 +\.foo +0+ +0+10000 +0+10000 .*
 *CONTENTS, ALLOC, LOAD, CODE
 *1 +\.bar +0+ +0+20000 +0+10000 .*
 *ALLOC, READONLY, CODE
