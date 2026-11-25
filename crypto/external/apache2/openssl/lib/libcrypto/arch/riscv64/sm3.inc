SM3_SRCS += sm3_riscv.c
SM3CPPFLAGS = -DOPENSSL_SM3_ASM
.include "../../sm3.inc"
