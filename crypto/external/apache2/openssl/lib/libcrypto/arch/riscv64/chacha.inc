.PATH.S: ${.PARSEDIR}

CHACHA_SRCS += chacha-riscv64-v-zbb.S chacha-riscv64-v-zvkb.S  
CHACHA_SRCS += chacha_riscv.c chacha_enc.c

CPPFLAGS+=-DINCLUDE_C_CHACHA20

.include "../../chacha.inc"
