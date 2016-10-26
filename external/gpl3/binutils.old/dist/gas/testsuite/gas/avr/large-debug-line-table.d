#name: AVR, large .debug_line table
#as: -mlink-relax -mmcu=avrxmega2
#objdump: --dwarf=decodedline
#source: large-debug-line-table.s
#target: avr-*-*

.*:     file format elf32-avr

Decoded dump of debug contents of section \.debug_line:

CU: large-debug-line-table\.c:
File name                            Line number    Starting address
large-debug-line-table\.c                       1                   0

#...
