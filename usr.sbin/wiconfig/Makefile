#	$NetBSD: Makefile,v 1.3.20.1 2008/09/18 04:30:16 wrstuden Exp $

PROG=	wiconfig
MAN=	wiconfig.8

.include <bsd.prog.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
COPTS+=	-fno-strict-aliasing
.endif
