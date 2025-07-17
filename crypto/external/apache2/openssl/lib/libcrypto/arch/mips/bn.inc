.include "mips.inc"

# Don't include mips.S for 32 bit mips because it does not work for mips1
.if ${MIPS_LE} && ${MIPS_64} == "64"
.PATH.S: ${.PARSEDIR}

BN_SRCS = mips${MIPS_64}.S
.endif

.include "../../bn.inc"
