.PATH.S: ${.PARSEDIR}

CHACHA_SRCS = chacha-armv4.S chacha-armv8-sve.S chacha-armv8.S

.include "../../chacha.inc"
