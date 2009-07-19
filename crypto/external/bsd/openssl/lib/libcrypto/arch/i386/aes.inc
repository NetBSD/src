.PATH.S: ${.PARSEDIR}
AES_SRCS = aes-586.S aesni-586.S
AESNI = yes
.include "../../aes.inc"
