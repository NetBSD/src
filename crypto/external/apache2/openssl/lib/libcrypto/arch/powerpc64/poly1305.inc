.PATH.S: ${.PARSEDIR}

POLY1305_SRCS = poly1305_ppc.c poly1305-ppc.S poly1305-ppcfp.S
POLY1305CPPFLAGS+=-DPOLY1305_ASM

.include "../../poly1305.inc"
