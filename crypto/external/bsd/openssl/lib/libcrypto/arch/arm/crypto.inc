.PATH.S: ${.PARSEDIR}

CPUID_SRCS += armv4cpuid.S armcap.c armv4-mont.S armv4-gf2m.S
CPUID = yes
CPPFLAGS += -DOPENSSL_BN_ASM_MONT -DOPENSSL_BN_ASM_GF2m

.include "../../crypto.inc"

