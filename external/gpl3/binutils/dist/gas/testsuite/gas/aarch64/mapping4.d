#objdump: --syms --special-syms
#name: AArch64 Mapping Symbols Test 4
# This test is only valid on ELF based ports.
#not-target: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd *-*-riscix*

.*: +file format.*aarch64.*

SYMBOL TABLE:
0[0]+00 l    d  .text	0[0]+00 .text
0[0]+00 l    d  .data	0[0]+00 .data
0[0]+00 l    d  .bss	0[0]+00 .bss
0[0]+00 l       .text	0[0]+00 \$x


