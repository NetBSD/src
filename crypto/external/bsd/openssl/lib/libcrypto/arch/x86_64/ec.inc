.PATH.S: ${.PARSEDIR}
EC_SRCS += \
ecp_nistz256-x86_64.S \
x25519-x86_64.S 

ECCPPFLAGS+= -DX25519_ASM

ECNI = yes
.include "../../ec.inc"
