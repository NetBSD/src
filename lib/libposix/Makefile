#	$NetBSD: Makefile,v 1.5 1999/02/24 15:10:16 drochner Exp $

LIB=	posix

CPPFLAGS+=-D_REENTRANT -I${.CURDIR}/../libc/include
.if ${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb"
ARCHDIR=	${.CURDIR}/../libc/arch/mips
.else
ARCHDIR=	${.CURDIR}/../libc/arch/${MACHINE_ARCH}
.endif
AINC=	-I${ARCHDIR}
.if defined(DESTDIR)
AINC+=	-nostdinc -idirafter ${DESTDIR}/usr/include
.endif

.include "${.CURDIR}/sys/Makefile.inc"

.include <bsd.lib.mk>
