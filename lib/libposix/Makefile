#	$NetBSD: Makefile,v 1.3 1997/05/31 21:21:50 cjs Exp $

LIB=	posix

CFLAGS+=-D_REENTRANT -I${.CURDIR}/../libc/include
AINC=	-I${.CURDIR}/../libc/arch/${MACHINE_ARCH}
.if defined(DESTDIR)
AINC+=	-nostdinc -idirafter ${DESTDIR}/usr/include
.endif

.include "${.CURDIR}/sys/Makefile.inc"

.include <bsd.lib.mk>
