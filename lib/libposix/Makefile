#	$NetBSD: Makefile,v 1.4 1997/10/23 02:58:54 lukem Exp $

LIB=	posix

CPPFLAGS+=-D_REENTRANT -I${.CURDIR}/../libc/include
AINC=	-I${.CURDIR}/../libc/arch/${MACHINE_ARCH}
.if defined(DESTDIR)
AINC+=	-nostdinc -idirafter ${DESTDIR}/usr/include
.endif

.include "${.CURDIR}/sys/Makefile.inc"

.include <bsd.lib.mk>
