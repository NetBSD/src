#source: plt_mapping_symbol.s
#ld: -shared -T relocs.ld -e0
#objdump: --syms --special-syms
#name: AArch64 mapping symbol for plt section test.
#...

SYMBOL TABLE:
#...
[0]+10010 l       .plt	0[0]+00 \$x
#...
