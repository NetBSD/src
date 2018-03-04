.PATH.S: ${.PARSEDIR}

CRYPTO_SRCS += mips-mont.S
CRYPTO_SRCS += libc-memequal.c

.include "../../crypto.inc"
