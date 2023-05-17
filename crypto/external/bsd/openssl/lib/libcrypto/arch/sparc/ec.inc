.if ${MACHINE} == "sparc64"
.PATH.S: ${.PARSEDIR}
EC_SRCS += \
	ecp_nistz256-sparcv9.S

ECCPPFLAGS+= -DECP_NISTZ256_ASM
ECNI = yes
AFLAGS.ecp_nistz256-sparcv9.S += -Wa,-Av9
.else
.endif

ECCPPFLAGS+= -DOPENSSL_NO_EC_NISTP_64_GCC_128

.include "../../ec.inc"
