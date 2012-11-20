#objdump: --syms --special-syms
#name: ARM Mapping Symbols

# Test the generation of ARM ELF Mapping Symbols

.*: +file format.*arm.*

SYMBOL TABLE:
0+00 l    d  .text	0+0 (|.text)
0+00 l    d  .data	0+0 (|.data)
0+00 l    d  .bss	0+0 (|.bss)
0+00 l     F .text	0+0 \$a
0+08 l     F .text	0+0 \$t
0+00 l     O .data	0+0 \$d
0+00 l    d  foo	0+0 (|foo)
0+00 l     F foo	0+0 \$t
0+00 g       .text	0+0 mapping
0+08 g     F .text	0+0 thumb_mapping
