#	$NetBSD: Makefile,v 1.3.2.1 1997/11/09 01:00:47 lukem Exp $

LIB=	posix

CPPFLAGS+=-D_REENTRANT -I${.CURDIR}/../libc/include
AINC=	-I${.CURDIR}/../libc/arch/${MACHINE_ARCH}
.if defined(DESTDIR)
AINC+=	-nostdinc -idirafter ${DESTDIR}/usr/include
.endif

.include "${.CURDIR}/sys/Makefile.inc"

.include <bsd.lib.mk>
