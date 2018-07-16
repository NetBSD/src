.PATH.S: ${.PARSEDIR}
CPUID_SRCS += arm64cpuid.S armcap.c
CPUID = yes
ARM_MAX_ARCH=8
CPPFLAGS += -D__ARM_MAX_ARCH__=${ARM_MAX_ARCH}
CFLAGS+= ${${ACTIVE_CC} == "clang" :? -no-integrated-as :}
.include "../../crypto.inc"

