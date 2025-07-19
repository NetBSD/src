.PATH.S: ${.PARSEDIR}
SHA_SRCS += sha256-riscv64-zvkb-zvknha_or_zvknhb.S sha512-riscv64-zvkb-zvknhb.S
SHACPPFLAGS+= -DSHA256_ASM -DSHA512_ASM
.include "../../sha.inc"
