.PATH.S: ${.PARSEDIR}
CPUID_SRCS = riscv32cpuid.S riscvcap.c
CPUID = yes
CPPFLAGS += -DOPENSSL_CPUID_OBJ -DUSE_ATOMIC_FALLBACKS
.include "../../crypto.inc"
