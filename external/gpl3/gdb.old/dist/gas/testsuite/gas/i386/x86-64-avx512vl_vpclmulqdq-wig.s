# Check 64bit AVX512VL,VPCLMULQDQ WIG instructions

	.allow_index_reg
	.text
_start:
	vpclmulqdq	$0xab, %xmm23, %xmm21, %xmm17	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	$123, 0x123(%rax,%r14,8), %xmm21, %xmm17	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	$123, 2032(%rdx), %xmm21, %xmm17	 # AVX512VL,VPCLMULQDQ Disp8
	vpclmulqdq	$0xab, %ymm19, %ymm18, %ymm23	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	$123, 0x123(%rax,%r14,8), %ymm18, %ymm23	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	$123, 4064(%rdx), %ymm18, %ymm23	 # AVX512VL,VPCLMULQDQ Disp8

	{evex} vpclmulqdq	$0xab, %xmm23, %xmm21, %xmm17	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	$123, 0x123(%rax,%r14,8), %xmm21, %xmm17	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	$123, 2032(%rdx), %xmm21, %xmm17	 # AVX512VL,VPCLMULQDQ Disp8
	{evex} vpclmulqdq	$0xab, %ymm19, %ymm18, %ymm23	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	$123, 0x123(%rax,%r14,8), %ymm18, %ymm23	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	$123, 4064(%rdx), %ymm18, %ymm23	 # AVX512VL,VPCLMULQDQ Disp8

	.intel_syntax noprefix
	vpclmulqdq	xmm18, xmm22, xmm17, 0xab	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	xmm18, xmm22, XMMWORD PTR [rax+r14*8+0x1234], 123	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	xmm18, xmm22, XMMWORD PTR [rdx+2032], 123	 # AVX512VL,VPCLMULQDQ Disp8
	vpclmulqdq	ymm26, ymm25, ymm23, 0xab	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	ymm26, ymm25, YMMWORD PTR [rax+r14*8+0x1234], 123	 # AVX512VL,VPCLMULQDQ
	vpclmulqdq	ymm26, ymm25, YMMWORD PTR [rdx+4064], 123	 # AVX512VL,VPCLMULQDQ Disp8

	{evex} vpclmulqdq	xmm18, xmm22, xmm17, 0xab	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	xmm18, xmm22, XMMWORD PTR [rax+r14*8+0x1234], 123	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	xmm18, xmm22, XMMWORD PTR [rdx+2032], 123	 # AVX512VL,VPCLMULQDQ Disp8
	{evex} vpclmulqdq	ymm26, ymm25, ymm23, 0xab	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	ymm26, ymm25, YMMWORD PTR [rax+r14*8+0x1234], 123	 # AVX512VL,VPCLMULQDQ
	{evex} vpclmulqdq	ymm26, ymm25, YMMWORD PTR [rdx+4064], 123	 # AVX512VL,VPCLMULQDQ Disp8
