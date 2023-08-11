.include "arm.inc"

.PATH.S: ${.PARSEDIR}
AES_SRCS += aes-armv4.S aes_cbc.c aesv8-armx.S

.if ${ARM_MAX_ARCH} >= 7
AES_SRCS+=bsaes-armv7.S
AESCPPFLAGS += -DBSAES_ASM
.endif

AESCPPFLAGS += -DAES_ASM
.include "../../aes.inc"
