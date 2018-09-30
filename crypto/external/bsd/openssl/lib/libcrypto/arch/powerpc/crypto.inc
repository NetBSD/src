CPUID_SRCS = ppccap.c ppccpuid.S
CRYPTOCPPFLAGS += -DOPENSSL_CPUID_OBJ
CPUID = yes
.include "../../crypto.inc"
