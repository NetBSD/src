.PATH.S: ${.PARSEDIR}

SHA_SRCS = sha1-alpha.S
SHACPPFLAGS = -DSHA1_ASM

.include "../../sha.inc"
