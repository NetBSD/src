#	$NetBSD: Makefile,v 1.5.2.1 2012/10/30 19:00:55 yamt Exp $

PROG=	wiconfig
MAN=	wiconfig.8

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_LLVM)
COPTS+=	-fno-strict-aliasing
.endif
