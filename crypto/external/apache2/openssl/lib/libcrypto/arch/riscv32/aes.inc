.PATH.S: ${.PARSEDIR}
AES_SRCS +=  aes_core.c aes_cbc.c aes-riscv32-zkn.S

AESNI = yes
.include "../../aes.inc"
