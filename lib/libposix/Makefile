#	$NetBSD: Makefile,v 1.2 1997/05/30 21:47:34 cjs Exp $

.include <bsd.own.mk>	# for BUILDDIR

LIB=	posix

CFLAGS+=-D_REENTRANT -I${.CURDIR}/../libc/include
AINC=	-I${.CURDIR}/../libc/arch/${MACHINE_ARCH}
.if defined(BUILDDIR)
AINC+=	-nostdinc -idirafter ${BUILDDIR}/usr/include
.endif

.include "${.CURDIR}/sys/Makefile.inc"

.include <bsd.lib.mk>
