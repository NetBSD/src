#	$NetBSD: bsd.oabi.mk,v 1.1 2013/04/27 08:44:35 matt Exp $

MLIBDIR=		oabi
.if ${MACHINE_ARCH:M*eb} != ""
ARM_MACHINE_ARCH=	armeb
LD+=			-m armelfb
.else
ARM_MACHINE_ARCH=	arm
LD+=			-m armelf
.endif
LIBC_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
LIBGCC_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
LIBEXECINFO_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
COMMON_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
KVM_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
PTHREAD_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
BFD_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
CSU_MACHINE_ARCH=	${ARM_MACHINE_ARCH}
CRYPTO_MACHINE_CPU=	${ARM_MACHINE_ARCH}
LDELFSO_MACHINE_CPU=	${ARM_MACHINE_ARCH}
GOMP_MACHINE_ARCH=	${ARM_MACHINE_ARCH}

COPTS+=		-mabi=apcs-gnu -mfloat-abi=soft
CPUFLAGS+=	-mabi=apcs-gnu -mfloat-abi=soft
LDADD+=		-mabi=apcs-gnu -mfloat-abi=soft
LDFLAGS+=	-mabi=apcs-gnu -mfloat-abi=soft
MKDEPFLAGS+=	-mabi=apcs-gnu -mfloat-abi=soft

.include "${.PARSEDIR}/../../Makefile.compat"
