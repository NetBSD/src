# Test -march=
	.text
# AES + AVX
vaesenc  (%ecx),%xmm0,%xmm2
# PCLMUL + AVX
vpclmulqdq $8,%xmm4,%xmm6,%xmm2
