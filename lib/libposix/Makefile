#	$NetBSD: Makefile,v 1.7 2001/01/04 09:00:24 itojun Exp $

LIB=	posix

.include <bsd.own.mk>

CPPFLAGS+=-D_REENTRANT -I${.CURDIR}/../libc/include
.if ${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb"
ARCHDIR=	${.CURDIR}/../libc/arch/mips
.elif ${MACHINE_ARCH} == "sh3el" || ${MACHINE_ARCH} == "sh3eb"
ARCHDIR=	${.CURDIR}/../libc/arch/sh3
.else
ARCHDIR=	${.CURDIR}/../libc/arch/${MACHINE_ARCH}
.endif
AINC=	-I${ARCHDIR}
.if defined(DESTDIR)
AINC+=	-nostdinc -idirafter ${DESTDIR}/usr/include
.endif

.include "${.CURDIR}/sys/Makefile.inc"

.include <bsd.lib.mk>
