.include "arm.inc"

.PATH.S: ${.PARSEDIR}
MODES_SRCS += ghash-armv4.S

MODES_SRCS += ghashv8-armx.S
AFLAGS.ghashv8-armx.S = -Wa,-march=armv8.4-a

MODESCPPFLAGS = -DGHASH_ASM
.include "../../modes.inc"
