.if empty(MACHINE_ARCH:M*eb)
.PATH.S: ${.PARSEDIR}

CRYPTO_SRCS += mips-mont${"${COPTS:M*-mabi=64*}" == "":?:64}.S
.endif
CRYPTO_SRCS += libc-memequal.c

.include "../../crypto.inc"
