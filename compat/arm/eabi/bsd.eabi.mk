#	$NetBSD: bsd.eabi.mk,v 1.2 2013/04/27 08:44:35 matt Exp $

MLIBDIR=		eabi
.if ${MACHINE_ARCH:M*eb} != ""
EARM_MACHINE_ARCH=	earmeb
LD+=			-m armelfb_nbsd_eabi
.else
EARM_MACHINE_ARCH=	earm
LD+=			-m armelf_nbsd_eabi
.endif
LIBC_MACHINE_ARCH=	${EARM_MACHINE_ARCH}
LIBGCC_MACHINE_ARCH=	${EARM_MACHINE_ARCH}
COMMON_MACHINE_ARCH=	${EARM_MACHINE_ARCH}
KVM_MACHINE_ARCH=	${EARM_MACHINE_ARCH}
PTHREAD_MACHINE_ARCH=	${EARM_MACHINE_ARCH}
BFD_MACHINE_ARCH=	${EARM_MACHINE_ARCH}
CSU_MACHINE_ARCH=	${EARM_MACHINE_ARCH}
CRYPTO_MACHINE_CPU=	${EARM_MACHINE_ARCH}
LDELFSO_MACHINE_CPU=	${EARM_MACHINE_ARCH}
GOMP_MACHINE_ARCH=	${EARM_MACHINE_ARCH}

COPTS+=		-mabi=aapcs-linux -mfloat-abi=soft -Wa,-meabi=5
CPUFLAGS+=	-mabi=aapcs-linux -mfloat-abi=soft -Wa,-meabi=5
LDADD+=		-mabi=aapcs-linux -mfloat-abi=soft -Wa,-meabi=5
LDFLAGS+=	-mabi=aapcs-linux -mfloat-abi=soft -Wa,-meabi=5
MKDEPFLAGS+=	-mabi=aapcs-linux -mfloat-abi=soft -Wa,-meabi=5

.include "${.PARSEDIR}/../../Makefile.compat"
