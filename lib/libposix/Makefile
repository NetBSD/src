#	$NetBSD: Makefile,v 1.9.2.1 2002/12/01 11:59:03 he Exp $

LIB=	posix

.include <bsd.own.mk>

# This library replaces routines in libc; no sense in warning about it.
LLIBS=

CPPFLAGS+=-D_REENTRANT -I${.CURDIR}/../libc/include
.if exists(${.CURDIR}/../libc/arch/${MACHINE_ARCH})
ARCHDIR=	${.CURDIR}/../libc/arch/${MACHINE_ARCH}
.elif exists(${.CURDIR}/../libc/arch/${MACHINE_CPU})
ARCHDIR=	${.CURDIR}/../libc/arch/${MACHINE_CPU}
.else
.BEGIN:
	@echo no ARCHDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
	@false
.endif

AINC=	-I${ARCHDIR}

.include "${.CURDIR}/sys/Makefile.inc"

.include <bsd.lib.mk>
