#source: attr-gnu-4-1.s
#source: attr-gnu-4-9.s -W
#ld: -r
#readelf: -A
#warning: Warning: .* uses -mdouble-float \(set by .*\), .* uses unknown floating point ABI 9

Attribute Section: gnu
File Attributes
  Tag_GNU_MIPS_ABI_FP: Hard float \(double precision\)

MIPS ABI Flags Version: 0

ISA: MIPS.*
GPR size: .*
CPR1 size: .*
CPR2 size: 0
FP ABI: Hard float \(double precision\)
ISA Extension: .*
ASEs:
	None
FLAGS 1: 0000000.
FLAGS 2: 00000000
