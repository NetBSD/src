.PATH.S: ${.PARSEDIR}
EC_SRCS += \
ecp_nistz256-ppc64.S \
ecp_nistp521-ppc64.S \
ecp_ppc.c \
x25519-ppc64.S

ECCPPFLAGS+= -DECP_NISTZ256_ASM
ECCPPFLAGS+= -DX25519_ASM
ECNI=yes

.include "../../ec.inc"
