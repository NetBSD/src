.include "arm.inc"

.PATH.S: ${.PARSEDIR}
AES_SRCS += aes-armv4.S aes_cbc.c

.if ${ARM_MAX_ARCH} >= 7
AES_SRCS+=bsaes-armv7.S
.elif ${ARM_MAX_ARCH} >= 8
AES_SRCS+=aesv8-armx.S
.endif

AESCPPFLAGS = -DAES_ASM -DBSAES_ASM
.include "../../aes.inc"
