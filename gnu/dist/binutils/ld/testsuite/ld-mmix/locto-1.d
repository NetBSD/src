#source: locto.s -globalize-symbols
#source: start.s
#ld: -m elf64mmix
#objdump: -str

.*:     file format elf64-mmix

SYMBOL TABLE:
0+1008 l    d  \.text	0+ (|\.text)
2000000000000000 l    d  \.data	0+ (|\.data)
2000000000000000 l    d  \.sbss	0+ (|\.sbss)
2000000000000000 l    d  \.bss	0+ (|\.bss)
0+ l    d  \*ABS\*	0+ (|\.shstrtab)
0+ l    d  \*ABS\*	0+ (|\.symtab)
0+ l    d  \*ABS\*	0+ (|\.strtab)
0+1008 g       \.text	0+ od
0+1010 g       \.text	0+ _start
0+1008 g       \*ABS\*	0+ __\.MMIX\.start\.\.text
2000000000000000 g       \*ABS\*	0+ __bss_start
2000000000000000 g       \*ABS\*	0+ _edata
2000000000000000 g       \*ABS\*	0+ _end
0+1010 g       \.text	0+ _start\.

Contents of section \.text:
 1008 00000000 00001008 e3fd0001           .*
