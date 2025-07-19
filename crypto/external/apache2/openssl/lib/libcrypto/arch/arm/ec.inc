.PATH.S: ${.PARSEDIR}

.if ${ARM_MAX_ARCH} >= 4
EC_SRCS += ecp_nistz256-armv4.S ecp_sm2p256-armv8.S
ECCPPFLAGS+= -DECP_NISTZ256_ASM
ECNI = yes
.endif
ECCPPFLAGS+= -DOPENSSL_NO_EC_NISTP_64_GCC_128

.include "../../ec.inc"
