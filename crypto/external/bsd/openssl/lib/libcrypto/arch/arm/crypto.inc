.PATH.S: ${.PARSEDIR}
CPUID_SRCS += armv4cpuid.S armcap.c armv4-mont.S armv4-gf2m.S
CPUID = yes
CPPFLAGS += -DOPENSSL_BN_ASM_MONT -DOPENSSL_BN_ASM_GF2m
.if !empty(MACHINE_ARCH:Mearmv4*) || ${MACHINE_ARCH} == "armeb"
ARM_MAX_ARCH=4
.elif ${MACHINE_ARCH} == "earmeb" || ${MACHINE_ARCH} == "earmhfeb"
ARM_MAX_ARCH=5
.elif !empty(MACHINE_ARCH:Mearmv6*)
ARM_MAX_ARCH=6
.else
ARM_MAX_ARCH=8
CPUID_SRCS += arm64cpuid.S
.endif
CPPFLAGS += -D__ARM_MAX_ARCH__=${ARM_MAX_ARCH}
.include "../../crypto.inc"

