#source: ifunc-3.s
#ld: -shared
#objdump: -dw
#target: aarch64*-*-*

#...
[ \t0-9a-f]+:[ \t0-9a-f]+bl[ \t0-9a-f]+<\*ABS\*\+(0x2e0|0x330)@plt>
#pass
