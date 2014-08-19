#source: ifunc-3-x86.s
#ld: -shared
#objdump: -dw
#target: x86_64-*-* i?86-*-*

#...
[ \t0-9a-f]+:[ \t0-9a-f]+call[ \t0-9a-fq]+<\*ABS\*(\+0x1b0|\+0x240|\+0x1a0|)@plt>
#pass
