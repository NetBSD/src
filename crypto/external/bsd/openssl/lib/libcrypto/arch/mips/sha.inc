.include "mips.inc"

.if ${MIPS_LE}
.PATH.S: ${.PARSEDIR}

SHA_SRCS += sha1-mips${MIPS_64}.S
SHA_SRCS += sha512-mips${MIPS_64}.S

SHACPPFLAGS = -DSHA1_ASM
.endif

.include "../../sha.inc"
