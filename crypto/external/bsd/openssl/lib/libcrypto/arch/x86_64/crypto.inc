.PATH.S: ${.PARSEDIR}
CPUID_SRCS = x86_64cpuid.S
CPUID = yes
CPPFLAGS += -DOPENSSL_CPUID_OBJ
.include "../../crypto.inc"
