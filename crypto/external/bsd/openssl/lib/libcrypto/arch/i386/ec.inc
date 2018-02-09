.PATH.S: ${.PARSEDIR}
EC_SRCS += \
ecp_nistz256-x86.S 

ECNI = yes
.include "../../ec.inc"
