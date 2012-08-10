#	$NetBSD: Makefile,v 1.6 2012/08/10 12:10:30 joerg Exp $

PROG=	wiconfig
MAN=	wiconfig.8

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_LLVM)
COPTS+=	-fno-strict-aliasing
.endif
