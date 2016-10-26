#as: --abi=32 --isa=shmedia
#objdump: -sr
#source: eh-1.s
#name: PR gas/6043

.*:     file format .*-sh64.*

RELOCATION RECORDS FOR \[\.eh_frame\]:
OFFSET  *TYPE  *VALUE 
00000000 R_SH_64_PCREL     \.text\+0x00000005


Contents of section .eh_frame:
 0000 00000000 00000000                    .*
