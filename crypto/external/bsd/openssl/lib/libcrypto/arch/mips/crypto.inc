.PATH.S: ${.PARSEDIR}

CRYPTO_SRCS += mips-mont${"${COPTS:M*-mabi=64*}" == "":?:64}.S
CRYPTO_SRCS += libc-memequal.c

.include "../../crypto.inc"
