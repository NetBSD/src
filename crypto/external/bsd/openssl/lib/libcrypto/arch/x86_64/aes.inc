.PATH.S: ${.PARSEDIR}
AES_SRCS = aes-x86_64.S aesni-x86_64.S bsaes-x86_64.S vpaes-x86_64.S
AESCPPFLAGS = -DAES_ASM -DVPAES_ASM -DBSAES_ASM -DOPENSSL_IA32_SSE2
AESNI = yes
.include "../../aes.inc"
