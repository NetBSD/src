.PATH.S: ${.PARSEDIR}
CPUID_SRCS = x86_64cpuid.S
CPUID = yes    
CRYPTOCPPFLAGS += -DOPENSSL_CPUID_OBJ
.include "../../crypto.inc"
