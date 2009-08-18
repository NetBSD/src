#source: attr-gnu-4-2.s
#source: attr-gnu-4-5.s
#as: -EB -32
#ld: -r -melf32btsmip
#readelf: -A
#warning: Warning: .* uses unknown floating point ABI 5
#target: mips*-*-*

Attribute Section: gnu
File Attributes
  Tag_GNU_MIPS_ABI_FP: Hard float \(-msingle-float\)
