#source: attr-gnu-4-3.s
#source: attr-gnu-4-8.s -W
#ld: -r
#readelf: -A
#warning: Warning: .* uses -msoft-float \(set by .*\), .* uses unknown floating point ABI 8

Attribute Section: gnu
File Attributes
  Tag_GNU_MIPS_ABI_FP: Soft float

MIPS ABI Flags Version: 0

ISA: MIPS.*
GPR size: .*
CPR1 size: .*
CPR2 size: 0
FP ABI: Soft float
ISA Extension: .*
ASEs:
	None
FLAGS 1: 0000000.
FLAGS 2: 00000000
