.PATH.S: ${.PARSEDIR}

SHA_SRCS += sha1-mips${"${COPTS:M*-mabi=64*}" == "":?:64}.S
SHA_SRCS += sha512-mips${"${COPTS:M*-mabi=64*}" == "":?:64}.S

SHACPPFLAGS = -DSHA1_ASM
.include "../../sha.inc"
