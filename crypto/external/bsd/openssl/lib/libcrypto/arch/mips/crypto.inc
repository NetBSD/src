.PATH.S: ${.PARSEDIR}

CRYPTO_SRCS += mips-mont${${MACHINE_ARCH:Mmips64*} == "":?:64}.S
CRYPTO_SRCS += libc-memequal.c

.include "../../crypto.inc"
