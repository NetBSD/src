#source: start3.s
#source: bpo-6.s
#source: bpo-2.s
#source: bpo-5.s
#as: -linker-allocated-gregs
#ld: -m elf64mmix --gc-sections
#objdump: -st

# Check that GC removes one of the three BPO:s, for the collected section.

.*:     file format elf64-mmix

SYMBOL TABLE:
0+ l    d  \.init	0+ (|\.init)
0+10 l    d  \.text	0+ (|\.text)
0+7e8 l    d  \.MMIX\.reg_contents	0+ (|\.MMIX\.reg_contents)
0+ l    df \*ABS\*	0+ .*
0+ l       \.init	0+ _start
0+ l    df \*ABS\*	0+ .*
2000000000000000 l       \.text	0+ __bss_start
2000000000000000 l       \.text	0+ _edata
2000000000000000 l       \.text	0+ _end
0+10 l       \.text	0+ _start\.
0+14 g       \.text	0+ x
0+10 g       \.text	0+ x2

Contents of section \.init:
 0000 00000000 0000003d 00000000 0000003a  .*
Contents of section \.text:
 0010 232dfe00 232dfd00                    .*
Contents of section \.MMIX\.reg_contents:
 07e8 00000000 0000107c 00000000 0000a420  .*
