#	$NetBSD: Makefile,v 1.40 2020/04/22 23:32:25 joerg Exp $

NOLIBCSANITIZER=	# defined
NOSANITIZER=		# defined

.include <bsd.own.mk>

CSU_MACHINE_ARCH?=	${MACHINE_ARCH}
CSU_MACHINE_CPU?=	${MACHINE_CPU}

.if !empty(CSU_MACHINE_ARCH:Mearm*)
ARCHDIR:=	${.CURDIR}/arch/earm
.elif exists(${.CURDIR}/arch/${CSU_MACHINE_ARCH}/Makefile.inc)
ARCHDIR:=	${.CURDIR}/arch/${CSU_MACHINE_ARCH}
.elif exists(${.CURDIR}/arch/${CSU_MACHINE_CPU}/Makefile.inc)
ARCHDIR:=	${.CURDIR}/arch/${CSU_MACHINE_CPU}
.else
.error Architecture (${CSU_MACHINE_ARCH} or ${CSU_MACHINE_CPU}) unsupported
.endif

# Ownership of globals from crt0.o is shared with libc for historic reasons
COPTS+=		-fcommon

.PATH:	${ARCHDIR}
.include "${ARCHDIR}/Makefile.inc"
.include "${.CURDIR}/common/Makefile.inc"
