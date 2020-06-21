.PATH.S: ${.PARSEDIR}
.PATH.c: ${.PARSEDIR}
CPUID = yes
CPUID_SRCS += sparccpuid.S sparcv9cap.c
CPPFLAGS += -DOPENSSL_CPUID_OBJ

CPUID_SRCS += sparcv9-mont.S sparcv9a-mont.S 
CPUID_SRCS += sparct4-mont.S vis3-mont.S
#CPPFLAGS += -DOPENSSL_BN_ASM_MONT
AFLAGS.sparcv9-mont.S+= -Wa,-Av9
AFLAGS.sparcv9a-mont.S+= -Wa,-Av9a
AFLAGS.sparct4-mont.S+= -Wa,-Av9a
AFLAGS.vis3-mont.S+= -Wa,-Av9a

.include "../../crypto.inc"
