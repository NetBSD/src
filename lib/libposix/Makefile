#	$NetBSD: Makefile,v 1.12.16.1 2008/01/09 01:36:28 matt Exp $

LIB=	posix

.include <bsd.own.mk>

# This library replaces routines in libc; no sense in warning about it.
LLIBS=

CPPFLAGS+=-D_REENTRANT -I${NETBSDSRCDIR}/lib/libc/include
CPPFLAGS+=-I${NETBSDSRCDIR}/sys
.if exists(${NETBSDSRCDIR}/lib/libc/arch/${MACHINE_ARCH})
ARCHDIR=	${NETBSDSRCDIR}/lib/libc/arch/${MACHINE_ARCH}
.elif exists(${NETBSDSRCDIR}/lib/libc/arch/${MACHINE_CPU})
ARCHDIR=	${NETBSDSRCDIR}/lib/libc/arch/${MACHINE_CPU}
.else
.BEGIN:
	@echo no ARCHDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
	@false
.endif

AFLAGS+=-I${ARCHDIR}

.include "${.CURDIR}/sys/Makefile.inc"

.include <bsd.lib.mk>
