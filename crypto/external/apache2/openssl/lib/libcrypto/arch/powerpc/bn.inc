.PATH.S: ${.PARSEDIR}
BN_SRCS = ppc.S ppc-mont.S bn_ppc.c
CPPFLAGS += -DOPENSSL_BN_ASM_MONT
.include "../../bn.inc"
