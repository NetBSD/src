# Test -march=
	.text
# cmov feature 
cmove	%eax,%ebx
# MMX
paddb %mm4,%mm3
# SSE
addss %xmm4,%xmm3
# SSE2
addsd %xmm4,%xmm3
# SSE3
addsubpd %xmm4,%xmm3
# SSSE3
phaddw %xmm4,%xmm3
# SSE4.1
phminposuw  %xmm1,%xmm3
# SSE4.2
crc32   %ecx,%ebx
# AVX
vzeroall
# VMX
vmxoff
# SMX
getsec
# Xsave
xgetbv
# AES
aesenc  (%ecx),%xmm0
# PCLMUL
pclmulqdq $8,%xmm1,%xmm0
# AES + AVX
vaesenc  (%ecx),%xmm0,%xmm2
# FMA
vfmaddpd %ymm4,%ymm6,%ymm2,%ymm7
# MOVBE
movbe   (%ecx),%ebx
# EPT
invept  (%ecx),%ebx
# 3DNow
pmulhrw %mm4,%mm3
# 3DNow Extensions
pswapd %mm4,%mm3
# SSE4a
insertq %xmm2,%xmm1
# SVME
vmload
# ABM
lzcnt %ecx,%ebx
# SSE5
frczss          %xmm2, %xmm1
# PadLock
xstorerng
