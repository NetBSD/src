.PATH.S: ${.PARSEDIR}
CPUID_SRCS = x86cpuid.S
CPUID = yes
CPPFLAGS += -DOPENSSL_CPUID_OBJ
COPTS.threads_pthread.c += -DBROKEN_CLANG_ATOMICS       # no atomic 64 bit ops

.include "../../crypto.inc"
