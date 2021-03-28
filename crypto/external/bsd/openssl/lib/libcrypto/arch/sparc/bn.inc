.PATH.S: ${.PARSEDIR}
# XXX bn-sparcv8plus.S doesn't work well. why?
BN_SRCS = bn-sparcv8.S
AFLAGS.bn-sparcv8.S+= -Wa,-Av9
.include "../../bn.inc"
