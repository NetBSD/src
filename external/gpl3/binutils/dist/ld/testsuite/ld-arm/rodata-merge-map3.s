@ This file contains the 3rd contribution, which is expected to
@ be partially merged into the 1st contribution (from
@ rodata-merge-map1.s), and generate a (redundant, but harmless)
@ $d mapping symbol.

        .section        .rodata.str1.1,"aMS",%progbits,1
.LC0:
        .string "foo"
        .string "world"
