#	$NetBSD: bsd.oabi.mk,v 1.4.16.1 2020/04/13 07:45:08 martin Exp $

.if !defined(MLIBDIR)
MLIBDIR=		oabi

.if ${MACHINE_ARCH} == "aarch64eb"
.error oabi is not supported on big endian AARCH64
.elif ${MACHINE_ARCH} == "aarch64"
ARM_MACHINE_ARCH=	arm
ARM_LD=			-m armelf_nbsd
LDFLAGS+=		-Wl,-m,armelf_nbsd
COPTS+=			-mcpu=cortex-a53
ARM_APCS_FLAGS= ${${ACTIVE_CC} == "clang":? -target arm--netbsdelf -B ${TOOLDIR}/aarch64--netbsd/bin :} -mabi=apcs-gnu -mfloat-abi=soft
ARM_APCS_FLAGS+= ${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 8:? -mno-thumb-interwork :}
.elif !empty(MACHINE_ARCH:M*eb)
ARM_MACHINE_ARCH=	armeb
ARM_LD=			-m armelfb_nbsd
LDFLAGS+=		-Wl,-m,armelfb_nbsd
.else
ARM_MACHINE_ARCH=	arm
ARM_LD=			-m armelf_nbsd
LDFLAGS+=		-Wl,-m,armelf_nbsd
.endif

LIBC_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
LIBGCC_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
LIBEXECINFO_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
LIBM_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
COMMON_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
KVM_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
PTHREAD_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
BFD_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
CSU_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
GOMP_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
XORG_MACHINE_ARCH=	${ARM_MACHINE_ARCH}

COMMON_MACHINE_CPU=	arm
COMPAT_MACHINE_CPU=	arm
CRYPTO_MACHINE_CPU=	arm
CSU_MACHINE_CPU=	arm
KVM_MACHINE_CPU=	arm
LIBC_MACHINE_CPU=	arm
LDELFSO_MACHINE_CPU=	arm
PTHREAD_MACHINE_CPU=	arm

MKSOFTFLOAT=	yes
COPTS+=		${ARM_APCS_FLAGS}
CPUFLAGS+=	${ARM_APCS_FLAGS}
LDFLAGS+=	${ARM_APCS_FLAGS}
MKDEPFLAGS+=	${ARM_APCS_FLAGS}

.include "${.PARSEDIR}/../../Makefile.compat"

.endif

.if empty(LD:M-m)
LD+=		${ARM_LD}
.endif
