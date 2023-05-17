.PATH.S: ${.PARSEDIR}

.if ${ARM_MAX_ARCH} >= 4
EC_SRCS += ecp_nistz256-armv4.S
ECCPPFLAGS+= -DECP_NISTZ256_ASM
ECNI = yes
.endif
ECCPPFLAGS+= -DOPENSSL_NO_EC_NISTP_64_GCC_128

.include "../../ec.inc"
