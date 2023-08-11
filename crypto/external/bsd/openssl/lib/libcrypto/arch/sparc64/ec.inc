.PATH.S: ${.PARSEDIR}
EC_SRCS += \
ecp_nistz256-sparcv9.S
ECCPPFLAGS+= -DECP_NISTZ256_ASM
ECCPPFLAGS+= -DOPENSSL_NO_EC_NISTP_64_GCC_128
ECNI = yes
COPTS.bn_exp.c+=-Wno-error=stack-protector
.include "../../ec.inc"
