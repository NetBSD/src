.PATH.S: ${.PARSEDIR}
AES_SRCS += \
aes-gcm-riscv64-zvkb-zvkg-zvkned.S \
aes-riscv64-zkn.S \
aes-riscv64-zvbb-zvkg-zvkned.S \
aes-riscv64-zvkb-zvkned.S \
aes-riscv64-zvkned.S \
aes-riscv64.S \
sm3-riscv64-zvksh.S \
sm4-riscv64-zvksed.S \
aes_cbc.c

AESCPPFLAGS = -DAES_ASM -DVPAES_ASM -DBSAES_ASM
AESNI = yes
.include "../../aes.inc"
