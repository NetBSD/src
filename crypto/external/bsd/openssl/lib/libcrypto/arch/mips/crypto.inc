.include "mips.inc"

.if ${MIPS_LE}
.PATH.S: ${.PARSEDIR}

CRYPTO_SRCS += mips-mont${MIPS_64}.S
.endif

.if ${MIPS_64} == ""
COPTS.threads_pthread.c += -DBROKEN_CLANG_ATOMICS       # no atomic 64 bit ops
.endif

.include "../../crypto.inc"
