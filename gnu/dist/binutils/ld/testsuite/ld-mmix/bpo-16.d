#source: start.s
#source: bpo-7.s
#source: bpo-7.s
#source: areg-t.s
#as: -linker-allocated-gregs
#ld: -m elf64mmix
#objdump: -st

# Two BPO:s against the same value get merged.

.*:     file format elf64-mmix

SYMBOL TABLE:
0+ l    d  \.text	0+ (|\.text)
2000000000000000 l    d  \.data	0+ (|\.data)
2000000000000000 l    d  \.sbss	0+ (|\.sbss)
2000000000000000 l    d  \.bss	0+ (|\.bss)
0+7f0 l    d  \.MMIX\.reg_contents	0+ (|\.MMIX\.reg_contents)
0+ l    d  \*ABS\*	0+ (|\.shstrtab)
0+ l    d  \*ABS\*	0+ (|\.symtab)
0+ l    d  \*ABS\*	0+ (|\.strtab)
0+ g       \.text	0+ _start
0+c g       \.text	0+ areg
#...

Contents of section \.text:
 0000 e3fd0001 234dfe00 234dfe00 fd040810  .*
Contents of section \.MMIX\.reg_contents:
 07f0 00000000 00000007                    .*
