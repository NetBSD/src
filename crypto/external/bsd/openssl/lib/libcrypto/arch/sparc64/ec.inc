.PATH.S: ${.PARSEDIR}
EC_SRCS += \
ecp_nistz256-sparcv9.S
ECCPPFLAGS+= -DECP_NISTZ256_ASM

ECNI = yes
.include "../../ec.inc"
