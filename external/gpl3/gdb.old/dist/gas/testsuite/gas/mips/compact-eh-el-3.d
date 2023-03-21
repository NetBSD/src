#objdump: -sr
#name: Compact EH EL #3 with personality id and large FDE data
#source: compact-eh-3.s
#as: -EL -mno-pdr

.*:     file format.*


RELOCATION RECORDS FOR \[.eh_frame_entry\]:
OFFSET   TYPE              VALUE 
00000000 R_MIPS_PC32       .text.*
00000004 R_MIPS_PC32       .gnu_extab


Contents of section .text:
 0000 00000000.*
Contents of section .reginfo:
 0000 00000000 00000000 00000000 00000000  .*
 0010 00000000 00000000                    .*
Contents of section .MIPS.abiflags:
 .*
 .*
Contents of section .gnu_extab:
 0000 02044003 055c                        .*
Contents of section .eh_frame_entry:
 0000 01000000 00000000                    .*
Contents of section .gnu.attributes:
 0000 410f0000 00676e75 00010700 00000401  .*
