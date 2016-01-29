#ld: -shared
#objdump: -dw
#target: aarch64*-*-*

#...
[ \t0-9a-f]+:[ \t0-9a-f]+bl[ \t0-9a-f]+<\*ABS\*\+(0x2c0|0x308)@plt>
[ \t0-9a-f]+:[ \t0-9a-f]+adrp[ \t]+x0, 0 <.*>
[ \t0-9a-f]+:[ \t0-9a-f]+add[ \t]+x0, x0, #(0x2b0|0x2f8)
#pass
