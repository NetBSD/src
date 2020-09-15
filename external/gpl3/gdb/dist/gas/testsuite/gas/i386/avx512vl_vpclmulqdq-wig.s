# Check 32bit AVX512VL,VPCLMULQDQ WIG instructions

	.allow_index_reg
	.text
_start:
	vpclmulqdq	$0xab, %xmm4, %xmm1, %xmm1	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	$123, -123456(%esp,%esi,8), %xmm1, %xmm1	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	$123, 2032(%edx), %xmm1, %xmm1	 # AVX512VL,VPCLMULQDQ Disp8
	vpclmulqdq	$0xab, %ymm2, %ymm5, %ymm3	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	$123, -123456(%esp,%esi,8), %ymm5, %ymm3	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	$123, 4064(%edx), %ymm5, %ymm3	 # AVX512VL,VPCLMULQDQ Disp8

	{evex} vpclmulqdq	$0xab, %xmm4, %xmm1, %xmm1	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	$123, -123456(%esp,%esi,8), %xmm1, %xmm1	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	$123, 2032(%edx), %xmm1, %xmm1	 # AVX512VL,VPCLMULQDQ Disp8
	{evex} vpclmulqdq	$0xab, %ymm2, %ymm5, %ymm3	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	$123, -123456(%esp,%esi,8), %ymm5, %ymm3	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	$123, 4064(%edx), %ymm5, %ymm3	 # AVX512VL,VPCLMULQDQ Disp8

	.intel_syntax noprefix
	vpclmulqdq	xmm6, xmm4, xmm1, 0xab	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	xmm6, xmm4, XMMWORD PTR [esp+esi*8-123456], 123	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	xmm6, xmm4, XMMWORD PTR [edx+2032], 123	 # AVX512VL,VPCLMULQDQ Disp8
	vpclmulqdq	ymm2, ymm4, ymm4, 0xab	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	ymm2, ymm4, YMMWORD PTR [esp+esi*8-123456], 123	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	ymm2, ymm4, YMMWORD PTR [edx+4064], 123	 # AVX512VL,VPCLMULQDQ Disp8

	{evex} vpclmulqdq	xmm6, xmm4, xmm1, 0xab	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	xmm6, xmm4, XMMWORD PTR [esp+esi*8-123456], 123	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	xmm6, xmm4, XMMWORD PTR [edx+2032], 123	 # AVX512VL,VPCLMULQDQ Disp8
	{evex} vpclmulqdq	ymm2, ymm4, ymm4, 0xab	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	ymm2, ymm4, YMMWORD PTR [esp+esi*8-123456], 123	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	ymm2, ymm4, YMMWORD PTR [edx+4064], 123	 # AVX512VL,VPCLMULQDQ Disp8
