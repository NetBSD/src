.PATH.S: ${.PARSEDIR}

CHACHA_SRCS = chacha-ppc.S chachap10-ppc.S chacha_enc.c

.include "../../chacha.inc"
