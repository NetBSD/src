.PATH.S: ${.PARSEDIR}
AES_SRCS = aes-x86_64.S aesni-x86_64.S
AESNI = yes
.include "../../aes.inc"
