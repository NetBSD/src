.PATH.S: ${.PARSEDIR}
#AES_SRCS = aesv8-armx.S aes_cbc.c vpaes-armv8.S
#AESCPPFLAGS = -DAES_ASM -DVPAES_ASM
AES_SRCS = aes_core.c aes_cbc.c vpaes-armv8.S
AESCPPFLAGS = -DVPAES_ASM
.include "../../aes.inc"
