.include "arm.inc"

.PATH.S: ${.PARSEDIR}
MODES_SRCS += ghash-armv4.S

.if ${ARM_MAX_ARCH} >= 8
MODES_SRCS += ghashv8-armx.S
.endif

MODESCPPFLAGS = -DGHASH_ASM
.include "../../modes.inc"
