.PATH.S: ${.PARSEDIR}
MODES_SRCS += aesni-gcm-x86_64.o ghash-x86_64.o
MODESCPPFLAGS = -DGHASH_ASM
.include "../../modes.inc"
