# Check 32bit AVX512VL,VAES WIG instructions

	.allow_index_reg
	.text
_start:
	vaesdec	%xmm4, %xmm5, %xmm6	 # AVX512VL,VAES
	vaesdec	-123456(%esp,%esi,8), %xmm5, %xmm6	 # AVX512VL,VAES
	vaesdec	%ymm4, %ymm5, %ymm6	 # AVX512VL,VAES
	vaesdec	-123456(%esp,%esi,8), %ymm5, %ymm6	 # AVX512VL,VAES

	vaesdeclast	%xmm4, %xmm5, %xmm6	 # AVX512VL,VAES
	vaesdeclast	-123456(%esp,%esi,8), %xmm5, %xmm6	 # AVX512VL,VAES
	vaesdeclast	%ymm4, %ymm5, %ymm6	 # AVX512VL,VAES
	vaesdeclast	-123456(%esp,%esi,8), %ymm5, %ymm6	 # AVX512VL,VAES

	vaesenc	%xmm4, %xmm5, %xmm6	 # AVX512VL,VAES
	vaesenc	-123456(%esp,%esi,8), %xmm5, %xmm6	 # AVX512VL,VAES
	vaesenc	%ymm4, %ymm5, %ymm6	 # AVX512VL,VAES
	vaesenc	-123456(%esp,%esi,8), %ymm5, %ymm6	 # AVX512VL,VAES

	vaesenclast	%xmm4, %xmm5, %xmm6	 # AVX512VL,VAES
	vaesenclast	-123456(%esp,%esi,8), %xmm5, %xmm6	 # AVX512VL,VAES
	vaesenclast	%ymm4, %ymm5, %ymm6	 # AVX512VL,VAES
	vaesenclast	-123456(%esp,%esi,8), %ymm5, %ymm6	 # AVX512VL,VAES

	.intel_syntax noprefix
	vaesdec	xmm6, xmm5, xmm4	 # AVX512VL,VAES
	vaesdec	xmm6, xmm5, XMMWORD PTR [esp+esi*8-123456]	 # AVX512VL,VAES
	vaesdec	ymm6, ymm5, ymm4	 # AVX512VL,VAES
	vaesdec	ymm6, ymm5, YMMWORD PTR [esp+esi*8-123456]	 # AVX512VL,VAES

	vaesdeclast	xmm6, xmm5, xmm4	 # AVX512VL,VAES
	vaesdeclast	xmm6, xmm5, XMMWORD PTR [esp+esi*8-123456]	 # AVX512VL,VAES
	vaesdeclast	ymm6, ymm5, ymm4	 # AVX512VL,VAES
	vaesdeclast	ymm6, ymm5, YMMWORD PTR [esp+esi*8-123456]	 # AVX512VL,VAES

	vaesenc	xmm6, xmm5, xmm4	 # AVX512VL,VAES
	vaesenc	xmm6, xmm5, XMMWORD PTR [esp+esi*8-123456]	 # AVX512VL,VAES
	vaesenc	ymm6, ymm5, ymm4	 # AVX512VL,VAES
	vaesenc	ymm6, ymm5, YMMWORD PTR [esp+esi*8-123456]	 # AVX512VL,VAES

	vaesenclast	xmm6, xmm5, xmm4	 # AVX512VL,VAES
	vaesenclast	xmm6, xmm5, XMMWORD PTR [esp+esi*8-123456]	 # AVX512VL,VAES
	vaesenclast	ymm6, ymm5, ymm4	 # AVX512VL,VAES
	vaesenclast	ymm6, ymm5, YMMWORD PTR [esp+esi*8-123456]	 # AVX512VL,VAES
