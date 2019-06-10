.PATH.S: ${.PARSEDIR}
EC_SRCS += \
ecp_nistz256-ppc64.S \
x25519-ppc64.S 

ECCPPFLAGS+= -DX25519_ASM

.include "../../ec.inc"
