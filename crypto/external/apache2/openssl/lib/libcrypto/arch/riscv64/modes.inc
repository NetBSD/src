.PATH.S: ${.PARSEDIR}
MODES_SRCS += ghash-riscv64-zvkb-zvbc.S ghash-riscv64-zvkg.S ghash-riscv64.S
MODESCPPFLAGS = -DGHASH_ASM
.include "../../modes.inc"
