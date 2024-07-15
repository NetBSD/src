.PATH.S: ${.PARSEDIR}
SHA_SRCS = sha1-x86_64.S sha1-mb-x86_64.S keccak1600-x86_64.S
SHACPPFLAGS = -DSHA1_ASM -DKECCAK1600_ASM
KECCAKNI = yes
.if 0
# This cannot be enabled until the SHA-2 symbol mess is resolved:
# https://mail-index.netbsd.org/tech-userlevel/2024/03/17/msg014265.html
# DO NOT TRY TO ENABLE IT, OR YOU MAY CAUSE NETBSD'S OPENSSL TO BE
# VULNERABLE TO REMOTE CODE EXECUTION BY STACK BUFFER OVERRUNS.
SHA_SRCS += sha512-x86_64.S sha256-mb-x86_64.S
SHACPPFLAGS+= -DSHA256_ASM -DSHA512_ASM
.endif
.include "../../sha.inc"
