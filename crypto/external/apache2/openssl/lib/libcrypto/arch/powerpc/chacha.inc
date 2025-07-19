.PATH.S: ${.PARSEDIR}

CHACHA_SRCS = chacha-ppc.S chacha_ppc.c chachap10-ppc.S

.include "../../chacha.inc"
