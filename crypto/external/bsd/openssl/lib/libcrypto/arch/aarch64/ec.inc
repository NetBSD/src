.PATH.S: ${.PARSEDIR}
EC_SRCS += ecp_nistz256-armv8.S 
ECNI = yes
.include "../../ec.inc"
