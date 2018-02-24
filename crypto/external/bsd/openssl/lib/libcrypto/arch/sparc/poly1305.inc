.PATH.S: ${.PARSEDIR}

POLY1305_SRCS = poly1305-sparcv9.S
POLY1305_CPPFLAGS+=-DPOLY1305_ASM

AFLAGS.poly1305-sparcv9.S+= -Wa,-Av9

.include "../../poly1305.inc"
