#objdump: -sr
#name: Compact EH EB #1 with personality ID and FDE data
#source: compact-eh-1.s
#as: -EB -mno-pdr

.*:     file format.*


RELOCATION RECORDS FOR \[.eh_frame_entry\]:
OFFSET   TYPE              VALUE 
00000000 R_MIPS_PC32       .text.*


Contents of section .text:
 0000 00000000.*
Contents of section .reginfo:
 0000 00000000 00000000 00000000 00000000  .*
 0010 00000000 00000000                    .*
Contents of section .MIPS.abiflags:
 .*
 .*
Contents of section .eh_frame_entry:
 0000 00000000 0104405c                    .*
Contents of section .gnu.attributes:
 0000 41000000 0f676e75 00010000 00070401  .*
