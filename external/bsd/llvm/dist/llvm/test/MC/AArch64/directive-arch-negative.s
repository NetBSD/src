// RUN: not llvm-mc -triple aarch64-unknown-none-eabi -filetype asm -o - %s 2>&1 | FileCheck %s

	.arch axp64
# CHECK: error: unknown arch name
# CHECK: 	.arch axp64
# CHECK:	      ^

	.arch armv8
	aese v0.8h, v1.8h

# CHECK: error: invalid operand for instruction
# CHECK: 	aese v0.8h, v1.8h
# CHECK:	^

// We silently ignore invalid features.
	.arch armv8+foo
	aese v0.8h, v1.8h

# CHECK: error: invalid operand for instruction
# CHECK:	aese v0.8h, v1.8h
# CHECK:	^

	.arch armv8+crypto

	.arch armv8

	aese v0.8h, v1.8h

# CHECK: error: invalid operand for instruction
# CHECK: 	aese v0.8h, v1.8h
# CHECK:	^

	.arch armv8.1-a+noras
	esb

# CHECK: error: instruction requires: ras
# CHECK:         esb

	.arch armv8.1-a+nolse
        casa  w5, w7, [x20]

# CHECK: error: instruction requires: lse
# CHECK:        casa  w5, w7, [x20]
