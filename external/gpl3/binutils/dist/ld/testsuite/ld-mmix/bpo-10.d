#source: start4.s
#source: bpo-6.s
#source: bpo-5.s
#as: -linker-allocated-gregs
#ld: -m elf64mmix --gc-sections
#objdump: -st

# Check that GC removes all (two) BPO:s when all are collected.

.*:     file format elf64-mmix

SYMBOL TABLE:
0+ l    d  \.init	0+ (|\.init)
0+7f8 l +d  \.MMIX.reg_contents	0+ (|\.MMIX\.reg_contents)
0+ l    df \*ABS\*	0+ .*
0+ l       \.init	0+ _start
0+ l    df \*ABS\*	0+ .*
2000000000000000 l       \.init	0+ __bss_start
2000000000000000 l       \.init	0+ _edata
2000000000000000 l       \.init	0+ _end
0+4 l       \.init	0+ _start\.

Contents of section \.init:
 0000 e37704a6                             .*
