.PATH.S: ${.PARSEDIR}

SHA_SRCS += sha1-mips${${MACHINE_ARCH:Mmips64*} == "":?:64}.S
SHA_SRCS += sha512-mips${${MACHINE_ARCH:Mmips64*} == "":?:64}.S

SHACPPFLAGS = -DSHA1_ASM
.include "../../sha.inc"
