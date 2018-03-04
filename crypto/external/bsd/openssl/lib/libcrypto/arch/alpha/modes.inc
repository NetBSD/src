.PATH.S: ${.PARSEDIR}

MODES_SRCS += ghash-alpha.S
MODESCPPFLAGS = -DGHASH_ASM

.include "../../modes.inc"
