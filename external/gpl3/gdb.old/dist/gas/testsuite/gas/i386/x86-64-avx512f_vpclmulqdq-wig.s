# Check 64bit AVX512F,VPCLMULQDQ WIG instructions

	.allow_index_reg
	.text
_start:
	vpclmulqdq	$0xab, %zmm19, %zmm20, %zmm22	 # AVX512F,VPCLMULQDQ
	vpclmulqdq	$123, 0x123(%rax,%r14,8), %zmm20, %zmm22	 # AVX512F,VPCLMULQDQ
	vpclmulqdq	$123, 8128(%rdx), %zmm20, %zmm22	 # AVX512F,VPCLMULQDQ Disp8

	.intel_syntax noprefix
	vpclmulqdq	zmm29, zmm28, zmm23, 0xab	 # AVX512F,VPCLMULQDQ
	vpclmulqdq	zmm29, zmm28, ZMMWORD PTR [rax+r14*8+0x1234], 123	 # AVX512F,VPCLMULQDQ
	vpclmulqdq	zmm29, zmm28, ZMMWORD PTR [rdx+8128], 123	 # AVX512F,VPCLMULQDQ Disp8
