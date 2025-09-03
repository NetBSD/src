CPUID_SRCS = ppccap.c ppccpuid.S
CPPFLAGS += -DOPENSSL_CPUID_OBJ
CPUID = yes

COPTS.threads_pthread.c += -DBROKEN_CLANG_ATOMICS       # no atomic 64 bit ops

.include "../../crypto.inc"
