.PATH.S: ${.PARSEDIR}
CPUID_SRCS = armv4cpuid.S armcap.c armv4-mont.S armv4-gf2m.S
CPUID = yes
CPPFLAGS += -DOPENSSL_BN_ASM_MONT -DOPENSSL_BN_ASM_GF2m
.if ${MACHINE_ARCH:M*armv4*} == ""
.	if ${MACHINE_ARCH} == "earmeb" || ${MACHINE_ARCH} == "earmhfeb"
CPPFLAGS += -D__ARM_MAX_ARCH__=5
.	else
CPPFLAGS += -D__ARM_MAX_ARCH__=8
.endif
.else
CPPFLAGS += -D__ARM_MAX_ARCH__=4
.endif
.include "../../crypto.inc"

