target:
	.insn r  0x33,  0,  0, a0, a1, a2
	.insn i  0x13,  0, a0, a1, 13
	.insn i  0x67,  0, a0, 10(a1)
	.insn i   0x3,  0, a0, 4(a1)
	.insn sb 0x63,  0, a0, a1, target
	.insn b  0x63,  0, a0, a1, target
	.insn s  0x23,  0, a0, 4(a1)
	.insn u  0x37, a0, 0xfff
	.insn uj 0x6f, a0, target
	.insn j  0x6f, a0, target

	.insn ci 0x1, 0x0, a0, 4
	.insn cr 0x2, 0x8, a0, a1
	.insn ciw 0x0, 0x0, a1, 1
	.insn cb 0x1, 0x6, a1, target
	.insn cj 0x1, 0x5, target

	.insn r  OP,  0,  0, a0, a1, a2
	.insn i  OP_IMM,  0, a0, a1, 13
	.insn i  JALR,  0, a0, 10(a1)
	.insn i  LOAD,  0, a0, 4(a1)
	.insn sb BRANCH,  0, a0, a1, target
	.insn b  BRANCH,  0, a0, a1, target
	.insn s  STORE,  0, a0, 4(a1)
	.insn u  LUI, a0, 0xfff
	.insn uj JAL, a0, target
	.insn j  JAL, a0, target

	.insn ci C1, 0x0, a0, 4
	.insn cr C2, 0x8, a0, a1
	.insn ciw C0, 0x0, a1, 1
	.insn ca C1, 0x23, 0x3, a0, a1
	.insn cb C1, 0x6, a1, target
	.insn cj C1, 0x5, target

	.insn r  MADD, 0, 0, a0, a1, a2, a3
	.insn r4 MADD, 0, 0, a0, a1, a2, a3
	.insn r4 MADD, 0, 0, fa0, a1, a2, a3
	.insn r4 MADD, 0, 0, fa0, fa1, a2, a3
	.insn r4 MADD, 0, 0, fa0, fa1, fa2, a3
	.insn r4 MADD, 0, 0, fa0, fa1, fa2, fa3
	.insn r  0x33,  0,  0, fa0, a1, a2
	.insn r  0x33,  0,  0, a0, fa1, a2
	.insn r  0x33,  0,  0, fa0, fa1, a2
	.insn r  0x33,  0,  0, a0, a1, fa2
	.insn r  0x33,  0,  0, fa0, a1, fa2
	.insn r  0x33,  0,  0, a0, fa1, fa2
	.insn r  0x33,  0,  0, fa0, fa1, fa2
