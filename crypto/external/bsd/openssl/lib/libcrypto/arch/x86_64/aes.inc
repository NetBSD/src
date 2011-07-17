.PATH.S: ${.PARSEDIR}
AES_SRCS = aes-x86_64.S aesni-x86_64.S
AESCPPFLAGS = -DAES_ASM -DOPENSSL_IA32_SSE2
AESNI = yes
.include "../../aes.inc"

COPTS.aesni-x86_64.S+=	${${ACTIVE_CC} == "clang":?-no-integrated-as:}
COPTS.aes-x86_64.S+=	${${ACTIVE_CC} == "clang":?-no-integrated-as:}
