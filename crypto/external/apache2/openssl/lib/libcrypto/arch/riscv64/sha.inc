.PATH.S: ${.PARSEDIR}
SHA_SRCS += sha256-riscv64-zvkb-zvknha_or_zvknhb.S sha512-riscv64-zvkb-zvknhb.S
SHA_SRCS += sha_riscv.c
SHACPPFLAGS+= -DSHA256_ASM -DSHA512_ASM -DINCLUDE_C_SHA256 -DINCLUDE_C_SHA512
.include "../../sha.inc"
