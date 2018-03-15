.PATH.S: ${.PARSEDIR}
SHA_SRCS = sha1-armv8.S sha512-armv8.S
SHACPPFLAGS = -DSHA1_ASM -DSHA256_ASM -DSHA512_ASM
.include "../../sha.inc"
