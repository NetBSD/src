#source: attr-gnu-4-3.s
#source: attr-gnu-4-5.s
#ld: -r
#readelf: -A
#warning: Warning: .* uses unknown floating point ABI 5
#target: mips*-*-*

Attribute Section: gnu
File Attributes
  Tag_GNU_MIPS_ABI_FP: Soft float
