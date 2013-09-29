#source: attr-gnu-4-4.s
#source: attr-gnu-4-5.s
#ld: -r
#warning: Warning: .* uses unknown floating point ABI 5
#target: mips*-*-*

Attribute Section: gnu
File Attributes
  Tag_GNU_MIPS_ABI_FP: 64-bit float \(-mips32r2 -mfp64\)
