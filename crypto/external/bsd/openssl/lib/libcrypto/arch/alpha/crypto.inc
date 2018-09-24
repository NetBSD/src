.PATH.S: ${.PARSEDIR}

ASMDIR:=${.PARSEDIR}

CPUID_SRCS += alpha-mont.S alphacpuid.S
CPUID = yes
CPPFLAGS += -DOPENSSL_BN_ASM_MONT -I${ASMDIR}
CRYPTOCPPFLAGS += -DOPENSSL_CPUID_OBJ

.include "../../crypto.inc"

