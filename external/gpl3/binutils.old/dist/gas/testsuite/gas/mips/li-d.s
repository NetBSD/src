# Source file used to test the li macro.

foo:
	.set mips1
	.set fp=32
	li.d $2, 0
	li.d $f0, 0
	.set mips2
	li.d $f0, 0
	.set fp=xx
	li.d $f0, 0
	.set mips32r2
	.set fp=32
	li.d $f0, 0
	.set fp=xx
	li.d $f0, 0
	.set fp=64
	li.d $f0, 0
	.set mips3
	li.d $f0, 0

# Force at least 8 (non-delay-slot) zero bytes, to make 'objdump' print ...
	.align  2
	.space  8
