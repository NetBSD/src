#	$NetBSD: Makefile,v 1.1 1997/05/18 20:14:21 kleink Exp $

LIB=	posix

CFLAGS+=-D_REENTRANT -I${.CURDIR}/../libc/include
AINC=	-I${.CURDIR}/../libc/arch/${MACHINE_ARCH}
.if defined(DESTDIR)
AINC+=	-nostdinc -idirafter ${DESTDIR}/usr/include
.endif

.include "${.CURDIR}/sys/Makefile.inc"

.include <bsd.lib.mk>
