.PATH.S: ${.PARSEDIR}
AES_SRCS += \
aes-riscv32-zkn.S

AESCPPFLAGS = -DAES_ASM -DVPAES_ASM -DBSAES_ASM
AESNI = yes
.include "../../aes.inc"
