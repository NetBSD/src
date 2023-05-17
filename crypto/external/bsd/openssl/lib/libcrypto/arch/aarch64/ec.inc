.PATH.S: ${.PARSEDIR}
ECCPPFLAGS+=-DECP_NISTZ256_ASM
EC_SRCS += ecp_nistz256-armv8.S
ECNI = yes
.include "../../ec.inc"
