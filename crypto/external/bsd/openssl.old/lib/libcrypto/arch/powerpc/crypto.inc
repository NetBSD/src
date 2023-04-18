CPUID_SRCS = ppccap.c ppccpuid.S
CPPFLAGS += -DOPENSSL_CPUID_OBJ
CPUID = yes
.include "../../crypto.inc"
