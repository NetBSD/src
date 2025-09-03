.PATH.S: ${.PARSEDIR}

CHACHA_SRCS = chacha-armv8.S chacha-armv8-sve.S

.include "../../chacha.inc"
