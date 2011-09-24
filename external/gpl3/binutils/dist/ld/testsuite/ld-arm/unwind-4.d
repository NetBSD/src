#ld: -T arm.ld
#objdump: -s

.*:     file format.*

#...
Contents of section .ARM.exidx:
 8020 (e0ffff7f b0b0a880 dcffff7f e8ffff7f|7fffffe0 80a8b0b0 7fffffdc 7fffffe8)  .*
 8030 (d8ffff7f b0b0a880 d8ffff7f 01000000|7fffffd8 80a8b0b0 7fffffd8 00000001)  .*
Contents of section .far:
#...
