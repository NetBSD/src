.PATH.S: ${.PARSEDIR}
MODES_SRCS = ghash-sparcv9.o
MODESCPPFLAGS = -DGHASH_ASM
.include "../../modes.inc"
