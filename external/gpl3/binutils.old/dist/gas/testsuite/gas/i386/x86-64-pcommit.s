# Check 32bit PCOMMIT instructions

	.allow_index_reg
	.text
_start:

	pcommit	 # PCOMMIT

	.intel_syntax noprefix
	pcommit	 # PCOMMIT
