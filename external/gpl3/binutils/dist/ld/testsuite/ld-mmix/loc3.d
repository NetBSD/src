#source: start.s
#source: loc1.s
#ld: -m elf64mmix
#objdump: -str

# Two text files in opposite order.

.*:     file format elf64-mmix

SYMBOL TABLE:
0+1000 l    d  \.text	0+ (|\.text)
0+1000 g       \.text	0+ _start
0+1004 g       \.text	0+ loc1
0+1000 g       \*ABS\*	0+ __\.MMIX\.start\.\.text
2000000000000000 g       \.text	0+ __bss_start
2000000000000000 g       \.text	0+ _edata
2000000000000000 g       \.text	0+ _end
0+1000 g       \.text	0+ _start\.

Contents of section \.text:
 1000 e3fd0001 fd030303                    .*
