.PATH.S: ${.PARSEDIR}
SHA_SRCS = sha1-sparcv9.S sha256-sparcv9.S sha512-sparcv9.S 
SHACPPFLAGS = -DSHA1_ASM -DSHA256_ASM -DSHA512_ASM
.include "../../sha.inc"
