#source: size32-1.s
#as: --64
#ld: -shared -melf_x86_64
#objdump: -R -s -j .data
#target: x86_64-*-*

.*: +file format .*

DYNAMIC RELOCATION RECORDS
OFFSET           TYPE              VALUE 
[[:xdigit:]]+ R_X86_64_SIZE32   xxx
[[:xdigit:]]+ R_X86_64_SIZE32   xxx-0x000000000000001e
[[:xdigit:]]+ R_X86_64_SIZE32   xxx\+0x000000000000001e
[[:xdigit:]]+ R_X86_64_SIZE32   yyy
[[:xdigit:]]+ R_X86_64_SIZE32   zzz


Contents of section .data:
 [[:xdigit:]]+ 00000000 00000000 00000000 00000000  ................
 [[:xdigit:]]+ 00000000 00000000 00000000 00000000  ................
 [[:xdigit:]]+ 00000000 00000000 00000000 00000000  ................
 [[:xdigit:]]+ 00000000 00000000 00000000           ............    
