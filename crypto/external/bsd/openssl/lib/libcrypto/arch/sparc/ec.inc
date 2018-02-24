.PATH.S: ${.PARSEDIR}
EC_SRCS += \

ECNI = yes
AFLAGS.ecp_nistz256-sparcv9.S += -Wa,-Av9

.include "../../ec.inc"
