#	$NetBSD: Makefile,v 1.8 2001/01/08 08:30:00 itojun Exp $

LIB=	posix

.include <bsd.own.mk>

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
.if defined(DESTDIR)
AINC+=	-nostdinc -idirafter ${DESTDIR}/usr/include
.endif

.include "${.CURDIR}/sys/Makefile.inc"

.include <bsd.lib.mk>
