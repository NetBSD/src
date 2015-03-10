.PATH.S: ${.PARSEDIR}
MODES_SRCS = ghash-armv4.S ghashv8-armx.S gcm128.c
MODESCPPFLAGS = -DGHASH_ASM
.include "../../modes.inc"
