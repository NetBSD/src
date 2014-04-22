#	$NetBSD: Makefile,v 1.34 2014/04/22 13:20:58 joerg Exp $

.include <bsd.own.mk>

CSU_MACHINE_ARCH?=	${MACHINE_ARCH}

.if !empty(CSU_MACHINE_ARCH:Mearm*)
ARCHDIR:=	${.CURDIR}/arch/earm
.elif exists(${.CURDIR}/arch/${CSU_MACHINE_ARCH}/Makefile.inc)
ARCHDIR:=	${.CURDIR}/arch/${CSU_MACHINE_ARCH}
.elif exists(${.CURDIR}/arch/${MACHINE_CPU}/Makefile.inc)
ARCHDIR:=	${.CURDIR}/arch/${MACHINE_CPU}
.else
.error Architecture (${CSU_MACHINE_ARCH} or ${MACHINE_CPU}) unsupported
.endif


.PATH:	${ARCHDIR}
.include "${ARCHDIR}/Makefile.inc"
.include "${.CURDIR}/common/Makefile.inc"
