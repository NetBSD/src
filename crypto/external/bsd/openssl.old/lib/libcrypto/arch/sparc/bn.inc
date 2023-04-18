.PATH.S: ${.PARSEDIR}
# Limit bn-sparcv8.S to sparc64; breaks on sparcstation 2 (sun4c)
.if ${MACHINE} == "sparc64"
# XXX bn-sparcv8plus.S doesn't work well. why?
BN_SRCS = bn-sparcv8.S
AFLAGS.bn-sparcv8.S+= -Wa,-Av9
.endif
.include "../../bn.inc"
