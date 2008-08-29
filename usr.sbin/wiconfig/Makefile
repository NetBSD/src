#	$NetBSD: Makefile,v 1.4 2008/08/29 00:02:26 gmcgarry Exp $

PROG=	wiconfig
MAN=	wiconfig.8

.include <bsd.prog.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
COPTS+=	-fno-strict-aliasing
.endif
