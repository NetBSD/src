#objdump : -s -j .data -j "\$DATA\$"
#name : relax .uleb128
# RISC-V doesn't support .sleb operands that are the difference of two symbols
# because symbol values are not known until after linker relaxation has been
# performed.
#notarget : riscv*-*-*

.*: .*

Contents of section .*
 0000 01020381 01000000 00000000 00000000.*
#...
 0080 00000004 ffff0500 06078380 01000000.*
#...
 4080 00000000 00000000 00000008 ffffffff.*
 4090 09090909 09090909 09090909 09090909.*
#pass
