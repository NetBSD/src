#	$NetBSD: bsd.eabihf.mk,v 1.2 2014/09/10 22:43:36 matt Exp $

.if !defined(MLIBDIR)

MLIBDIR=		eabihf

EARM_COMPAT_FLAGS=	-mfloat-abi=hard
EARM_COMPAT_FLAGS+=	-mabi=aapcs-linux
MKSOFTFLOAT=no

.if ${MACHINE_ARCH} == "aarch64eb"
EARM_COMPAT_FLAGS+=	-target armeb--netbsdelf-eabihf
EARM_COMPAT_FLAGS+=	-mcpu=cortex-a53
ARM_MACHINE_ARCH=	earmv7hfeb
LDFLAGS+=		-Wl,--be8
ARM_LD=			-m armelfb_nbsd_eabihf --be8
.elif ${MACHINE_ARCH} == "aarch64"
EARM_COMPAT_FLAGS+=	-target arm--netbsdelf-eabihf
EARM_COMPAT_FLAGS+=	-mcpu=cortex-a53
ARM_MACHINE_ARCH=	earmv7hf
ARM_LD=			-m armelf_nbsd_eabihf
.elif !empty(MACHINE_ARCH:M*eb)
EARM_COMPAT_FLAGS+=	-target armeb--netbsdelf-eabihf
ARM_MACHINE_ARCH=	earmhfeb
ARM_LD=			-m armelfb_nbsd_eabihf
.else
EARM_COMPAT_FLAGS+=	-target arm--netbsdelf-eabihf
ARM_MACHINE_ARCH=	earmhf
ARM_LD=			-m armelf_nbsd_eabihf
.endif

EARM_COMPAT_FLAGS+=	-B ${TOOLDIR}/aarch64--netbsd/bin 

LIBC_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
LIBGCC_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
LIBEXECINFO_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
LIBM_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
COMMON_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
KVM_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
PTHREAD_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
BFD_MACHINE_ARCH=	earmhf
CSU_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
GOMP_MACHINE_ARCH=	${ARM_MACHINE_ARCH}

COMMON_MACHINE_CPU=	arm
COMPAT_MACHINE_CPU=	arm
CRYPTO_MACHINE_CPU=	arm
CSU_MACHINE_CPU=	arm
KVM_MACHINE_CPU=	arm
LDELFSO_MACHINE_CPU=	arm
LIBC_MACHINE_CPU=	arm
PTHREAD_MACHINE_CPU=	arm

.if defined(ACTIVE_CC)
EARM_COMPAT_FLAGS+=	${${ACTIVE_CC} == "gcc":?-Wa,-meabi=5:}
.endif

COPTS+=			${EARM_COMPAT_FLAGS}
CPUFLAGS+=		${EARM_COMPAT_FLAGS}
LDADD+=			${EARM_COMPAT_FLAGS}
LDFLAGS+=		${EARM_COMPAT_FLAGS}
MKDEPFLAGS+=		${EARM_COMPAT_FLAGS}

.include "${.PARSEDIR}/../../Makefile.compat"

.endif

.if empty(LD:M-m)
LD+=			${ARM_LD}
.endif
