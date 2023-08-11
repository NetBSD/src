.include "arm.inc"
.PATH.S: ${.PARSEDIR}

CPUID_SRCS += armv4cpuid.S armcap.c armv4-mont.S armv4-gf2m.S
CPUID = yes
CPPFLAGS += -DOPENSSL_BN_ASM_MONT -DOPENSSL_BN_ASM_GF2m
CPPFLAGS += -DOPENSSL_CPUID_OBJ

.if ${ARM_MAX_ARCH} <= 5
COPTS.threads_pthread.c += -DBROKEN_CLANG_ATOMICS       # no atomic 64 bit ops
.endif

.include "../../crypto.inc"

