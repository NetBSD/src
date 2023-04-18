.if ${MACHINE} == "sparc64"
.PATH.S: ${.PARSEDIR}
EC_SRCS += \
	ecp_nistz256-sparcv9.S

ECNI = yes
AFLAGS.ecp_nistz256-sparcv9.S += -Wa,-Av9
.else
.endif

.include "../../ec.inc"
