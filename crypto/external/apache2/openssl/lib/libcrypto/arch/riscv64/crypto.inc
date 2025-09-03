.PATH.S: ${.PARSEDIR}
CPUID_SRCS = riscv64cpuid.S riscvcap.c
CPUID = yes
CPPFLAGS += -DOPENSSL_CPUID_OBJ
.include "../../crypto.inc"
