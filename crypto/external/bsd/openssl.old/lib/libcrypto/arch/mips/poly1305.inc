.include "mips.inc"

.if ${MIPS_LE} && ${MIPS_64} == "64"

.PATH.S: ${.PARSEDIR}

POLY1305_SRCS = poly1305-mips64.S
POLY1305_CPPFLAGS+=-DPOLY1305_ASM

.endif

.include "../../poly1305.inc"
