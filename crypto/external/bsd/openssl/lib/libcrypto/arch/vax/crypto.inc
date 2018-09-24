.PATH.S: ${.PARSEDIR}

CRYPTO_SRCS += libc-memequal.c
CRYPTOCPPFLAGS += -DOPENSSL_CPUID_OBJ

.include "../../crypto.inc"
