.include "mips.inc"
.if ${MIPS_LE}
.PATH.S: ${.PARSEDIR}

#AES_SRCS = aes-mips${MIPS_64}.S
#AESCPPFLAGS = -DAES_ASM
.endif

.include "../../aes.inc"
