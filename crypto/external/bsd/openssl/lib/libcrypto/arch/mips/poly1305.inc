.if empty(MACHINE_ARCH:M*eb)
.if !empty(MACHINE_ARCH:Mmips64*)

.PATH.S: ${.PARSEDIR}

POLY1305_SRCS = poly1305-mips64.S
POLY1305_CPPFLAGS+=-DPOLY1305_ASM

.endif
.endif

.include "../../poly1305.inc"
