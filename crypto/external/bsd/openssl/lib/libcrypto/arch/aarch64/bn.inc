.PATH.S: ${.PARSEDIR}
BN_SRCS = armv8-mont.S bn_asm.c
CPPFLAGS += -DOPENSSL_BN_ASM_MONT
.include "../../bn.inc"
