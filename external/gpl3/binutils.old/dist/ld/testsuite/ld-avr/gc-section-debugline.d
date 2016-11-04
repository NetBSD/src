#name: dwarf decoded line after gc-sections
#as: -mmcu=avr51 -gdwarf-sections -g
#ld: -mavr51 -gc-sections -u main
#objdump: --dwarf=decodedline
#source: per-function-debugline.s
#target: avr-*-*

.*:     file format elf32-avr

Decoded dump of debug contents of section .debug_line:

CU: .*:
File name                            Line number    Starting address
per-function-debugline.s                      39                   0

per-function-debugline.s                      40                 0x2

per-function-debugline.s                      41                 0x4

per-function-debugline.s                      42                 0x6

per-function-debugline.s                      47                 0x8

per-function-debugline.s                      48                 0xc

per-function-debugline.s                      49                0x10

per-function-debugline.s                      50                0x12

per-function-debugline.s                      51                0x16

per-function-debugline.s                      52                0x1a

per-function-debugline.s                      54                0x1c

per-function-debugline.s                      55                0x1e

per-function-debugline.s                      56                0x20

per-function-debugline.s                      62                0x22

per-function-debugline.s                      63                0x24

per-function-debugline.s                      64                0x26

per-function-debugline.s                      65                0x28

per-function-debugline.s                      70                0x2a

per-function-debugline.s                      71                0x2e

per-function-debugline.s                      72                0x32

per-function-debugline.s                      73                0x36

per-function-debugline.s                      74                0x38

per-function-debugline.s                      75                0x3c

per-function-debugline.s                      76                0x40

per-function-debugline.s                      77                0x44

per-function-debugline.s                      79                0x48

per-function-debugline.s                      80                0x4a

per-function-debugline.s                      81                0x4c


