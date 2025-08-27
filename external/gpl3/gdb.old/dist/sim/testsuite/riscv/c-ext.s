# Basic Tests for C extension.
# mach: riscv32 riscv64
# sim(riscv32): --model RV32IC
# sim(riscv64): --model RV64IC
# ld(riscv32): -m elf32lriscv
# ld(riscv64): -m elf64lriscv
# as(riscv32): -march=rv32ic
# as(riscv64): -march=rv64ic

.include "testutils.inc"

	.data
	.align 4
_data:
	.word	1234
	.word	0

	start
	la	a0, _data

	# Test load-store instructions.
	c.lw	a1,0(a0)
	c.sw	a1,4(a0)
	c.lw	a2,4(a0)

	li	a5,1234
	bne	a1,a5,test_fail
	bne	a2,a5,test_fail

	# Test basic arithmetic.
	c.li	a0,0
	c.li	a1,1
	c.addi	a0,1
	c.addi	a0,-1
	c.add	a0,a1
	c.sub	a0,a1

	li	a5,1
	bne	a0,x0,test_fail
	bne	a1,a5,test_fail

	# Test logical operations.
	c.li	a0,7
	c.li	a1,7
	c.li	a2,4
	c.li	a3,3
	c.li	a4,3
	c.andi	a0,3
	c.and	a1,a0
	c.or	a2,a3
	c.xor	a4,a4

	li	a5,3
	bne	a0,a5,test_fail
	bne	a1,a5,test_fail
	bne	a4,x0,test_fail
	li	a5,7
	bne	a2,a5,test_fail

	# Test shift operations.
	c.li	a0,4
	c.li	a1,4
	c.slli	a0,1
	c.srli	a1,1

	li	a5,8
	bne	a0,a5,test_fail
	li	a5,2
	bne	a1,a5,test_fail

	# Test jump instruction.
	c.j	1f

	j	test_fail
1:
	la	a0,2f

	# Test jump register instruction.
	c.jr	a0

	j	test_fail

2:
	# Test branch instruction.
	c.li	a0,1
	c.beqz	a0,test_fail
	c.li	a0,0
	c.bnez	a0,test_fail

test_pass:
	pass
	fail

test_fail:
	fail
