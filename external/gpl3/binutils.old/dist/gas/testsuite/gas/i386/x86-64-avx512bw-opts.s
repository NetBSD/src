# Check 64bit AVX512BW swap instructions

	.allow_index_reg
	.text
_start:
	vpextrw	$0xab, %xmm29, %rax	 # AVX512BW
	vpextrw.s	$0xab, %xmm29, %rax	 # AVX512BW
	vpextrw	$123, %xmm29, %rax	 # AVX512BW
	vpextrw.s	$123, %xmm29, %rax	 # AVX512BW
	vpextrw	$123, %xmm29, %r8	 # AVX512BW
	vpextrw.s	$123, %xmm29, %r8	 # AVX512BW
	vpextrw	$0xab, %xmm29, %rax	 # AVX512BW
	vpextrw.s	$0xab, %xmm29, %rax	 # AVX512BW
	vpextrw	$123, %xmm29, %rax	 # AVX512BW
	vpextrw.s	$123, %xmm29, %rax	 # AVX512BW
	vpextrw	$123, %xmm29, %r8	 # AVX512BW
	vpextrw.s	$123, %xmm29, %r8	 # AVX512BW
	vmovdqu8	%zmm29, %zmm30	 # AVX512BW
	vmovdqu8.s	%zmm29, %zmm30	 # AVX512BW
	vmovdqu8	%zmm29, %zmm30{%k7}	 # AVX512BW
	vmovdqu8.s	%zmm29, %zmm30{%k7}	 # AVX512BW
	vmovdqu8	%zmm29, %zmm30{%k7}{z}	 # AVX512BW
	vmovdqu8.s	%zmm29, %zmm30{%k7}{z}	 # AVX512BW
	vmovdqu8	%zmm29, %zmm30	 # AVX512BW
	vmovdqu8.s	%zmm29, %zmm30	 # AVX512BW
	vmovdqu8	%zmm29, %zmm30{%k7}	 # AVX512BW
	vmovdqu8.s	%zmm29, %zmm30{%k7}	 # AVX512BW
	vmovdqu8	%zmm29, %zmm30{%k7}{z}	 # AVX512BW
	vmovdqu8.s	%zmm29, %zmm30{%k7}{z}	 # AVX512BW
	vmovdqu16	%zmm29, %zmm30	 # AVX512BW
	vmovdqu16.s	%zmm29, %zmm30	 # AVX512BW
	vmovdqu16	%zmm29, %zmm30{%k7}	 # AVX512BW
	vmovdqu16.s	%zmm29, %zmm30{%k7}	 # AVX512BW
	vmovdqu16	%zmm29, %zmm30{%k7}{z}	 # AVX512BW
	vmovdqu16.s	%zmm29, %zmm30{%k7}{z}	 # AVX512BW
	vmovdqu16	%zmm29, %zmm30	 # AVX512BW
	vmovdqu16.s	%zmm29, %zmm30	 # AVX512BW
	vmovdqu16	%zmm29, %zmm30{%k7}	 # AVX512BW
	vmovdqu16.s	%zmm29, %zmm30{%k7}	 # AVX512BW
	vmovdqu16	%zmm29, %zmm30{%k7}{z}	 # AVX512BW
	vmovdqu16.s	%zmm29, %zmm30{%k7}{z}	 # AVX512BW

	.intel_syntax noprefix
	vpextrw	rax, xmm29, 0xab	 # AVX512BW
	vpextrw.s	rax, xmm29, 0xab	 # AVX512BW
	vpextrw	rax, xmm29, 123	 # AVX512BW
	vpextrw.s	rax, xmm29, 123	 # AVX512BW
	vpextrw	r8, xmm29, 123	 # AVX512BW
	vpextrw.s	r8, xmm29, 123	 # AVX512BW
	vpextrw	rax, xmm29, 0xab	 # AVX512BW
	vpextrw.s	rax, xmm29, 0xab	 # AVX512BW
	vpextrw	rax, xmm29, 123	 # AVX512BW
	vpextrw.s	rax, xmm29, 123	 # AVX512BW
	vpextrw	r8, xmm29, 123	 # AVX512BW
	vpextrw.s	r8, xmm29, 123	 # AVX512BW
	vmovdqu8	zmm30, zmm29	 # AVX512BW
	vmovdqu8.s	zmm30, zmm29	 # AVX512BW
	vmovdqu8	zmm30{k7}, zmm29	 # AVX512BW
	vmovdqu8.s	zmm30{k7}, zmm29	 # AVX512BW
	vmovdqu8	zmm30{k7}{z}, zmm29	 # AVX512BW
	vmovdqu8.s	zmm30{k7}{z}, zmm29	 # AVX512BW
	vmovdqu8	zmm30, zmm29	 # AVX512BW
	vmovdqu8.s	zmm30, zmm29	 # AVX512BW
	vmovdqu8	zmm30{k7}, zmm29	 # AVX512BW
	vmovdqu8.s	zmm30{k7}, zmm29	 # AVX512BW
	vmovdqu8	zmm30{k7}{z}, zmm29	 # AVX512BW
	vmovdqu8.s	zmm30{k7}{z}, zmm29	 # AVX512BW
	vmovdqu16	zmm30, zmm29	 # AVX512BW
	vmovdqu16.s	zmm30, zmm29	 # AVX512BW
	vmovdqu16	zmm30{k7}, zmm29	 # AVX512BW
	vmovdqu16.s	zmm30{k7}, zmm29	 # AVX512BW
	vmovdqu16	zmm30{k7}{z}, zmm29	 # AVX512BW
	vmovdqu16.s	zmm30{k7}{z}, zmm29	 # AVX512BW
	vmovdqu16	zmm30, zmm29	 # AVX512BW
	vmovdqu16.s	zmm30, zmm29	 # AVX512BW
	vmovdqu16	zmm30{k7}, zmm29	 # AVX512BW
	vmovdqu16.s	zmm30{k7}, zmm29	 # AVX512BW
	vmovdqu16	zmm30{k7}{z}, zmm29	 # AVX512BW
	vmovdqu16.s	zmm30{k7}{z}, zmm29	 # AVX512BW
