.include "mips.inc"

.if ${MIPS_LE}
.PATH.S: ${.PARSEDIR}

BN_SRCS = mips${MIPS_64}.S
.endif

.include "../../bn.inc"
