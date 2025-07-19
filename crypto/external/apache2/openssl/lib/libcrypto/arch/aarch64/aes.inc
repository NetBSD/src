.PATH.S: ${.PARSEDIR}
AESCPPFLAGS = -DVPAES_ASM
AES_SRCS += aes_core.c aes_cbc.c vpaes-armv8.S aesv8-armx.S aes-gcm-armv8_64.S
AES_SRCS += aes-gcm-armv8-unroll8_64.S bsaes-armv8.S

.include "../../aes.inc"
