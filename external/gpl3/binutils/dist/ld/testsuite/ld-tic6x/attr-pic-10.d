#name: C6X PIC attribute merging, 1 0
#as: -mlittle-endian
#ld: -r -melf32_tic6x_le
#source: attr-pic-1.s
#source: attr-pic-0.s
#warning: .*differ in position-dependence of code addressing
