.PATH.S: ${.PARSEDIR}
AES_SRCS = aes-m68k.S aes_cbc.c
AESCPPFLAGS = -DAES_ASM
.include "../../aes.inc"
