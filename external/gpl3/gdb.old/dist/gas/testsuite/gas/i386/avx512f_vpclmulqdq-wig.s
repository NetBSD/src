# Check 32bit AVX512F,VPCLMULQDQ WIG instructions

	.allow_index_reg
	.text
_start:
	vpclmulqdq	$0xab, %zmm2, %zmm1, %zmm6	 # AVX512F,VPCLMULQDQ
	vpclmulqdq	$123, -123456(%esp,%esi,8), %zmm1, %zmm6	 # AVX512F,VPCLMULQDQ
	vpclmulqdq	$123, 8128(%edx), %zmm1, %zmm6	 # AVX512F,VPCLMULQDQ Disp8

	.intel_syntax noprefix
	vpclmulqdq	zmm5, zmm1, zmm2, 0xab	 # AVX512F,VPCLMULQDQ
	vpclmulqdq	zmm5, zmm1, ZMMWORD PTR [esp+esi*8-123456], 123	 # AVX512F,VPCLMULQDQ
	vpclmulqdq	zmm5, zmm1, ZMMWORD PTR [edx+8128], 123	 # AVX512F,VPCLMULQDQ Disp8
