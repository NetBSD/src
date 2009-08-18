#source: attr-gnu-4-4.s
#source: attr-gnu-4-0.s
#as: -EB -32
#ld: -r -melf32btsmip
#readelf: -A
#target: mips*-*-*

Attribute Section: gnu
File Attributes
  Tag_GNU_MIPS_ABI_FP: 64-bit float \(-mips32r2 -mfp64\)
