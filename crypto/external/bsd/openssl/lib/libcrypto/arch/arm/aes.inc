.PATH.S: ${.PARSEDIR}
AES_SRCS = aes-armv4.S aesv8-armx.S bsaes-armv7.S aes_cbc.c
AESCPPFLAGS = -DAES_ASM -DBSAES_ASM
AFLAGS.aes-armv4.S= -fno-integrated-as
.include "../../aes.inc"
