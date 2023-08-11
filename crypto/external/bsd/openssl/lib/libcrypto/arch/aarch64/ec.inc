.PATH.S: ${.PARSEDIR}
ECCPPFLAGS+=-DECP_NISTZ256_ASM
EC_SRCS += ecp_nistz256-armv8.S
ECNI = yes
.if ${MACHINE_ARCH} == "aarch64eb"
ECCPPFLAGS+=-DOPENSSL_NO_EC_NISTP_64_GCC_128
.endif
.include "../../ec.inc"
