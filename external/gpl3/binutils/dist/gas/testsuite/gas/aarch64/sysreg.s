
	# Test case for system registers
	.text

	msr pmovsclr_el0, x7
	mrs x0, pmovsclr_el0

	msr pmovsset_el0, x7
	mrs x0, pmovsset_el0

	mrs x0, id_dfr0_el1
	mrs x0, id_pfr0_el1
	mrs x0, id_pfr1_el1
	mrs x0, id_afr0_el1
	mrs x0, id_mmfr0_el1
	mrs x0, id_mmfr1_el1
	mrs x0, id_mmfr2_el1
	mrs x0, id_mmfr3_el1
	mrs x0, id_isar0_el1
	mrs x0, id_isar1_el1
	mrs x0, id_isar2_el1
	mrs x0, id_isar3_el1
	mrs x0, id_isar4_el1
	mrs x0, id_isar5_el1
