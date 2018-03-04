.PATH.S: ${.PARSEDIR}
SHA_SRCS = sha1-mips.S sha512-mips.S
SHACPPFLAGS = -DSHA1_ASM
.include "../../sha.inc"
