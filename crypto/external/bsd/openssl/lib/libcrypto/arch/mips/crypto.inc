.include "mips.inc"

.if ${MIPS_LE}
.PATH.S: ${.PARSEDIR}

CRYPTO_SRCS += mips-mont${MIPS_64}.S
.endif

.include "../../crypto.inc"
