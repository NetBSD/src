.PATH.S: ${.PARSEDIR}
MODES_SRCS += modexp512-x86_64.S ghash-x86_64.o
MODESCPPFLAGS = -DGHASH_ASM
.include "../../modes.inc"
