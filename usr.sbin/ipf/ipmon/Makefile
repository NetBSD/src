#	$NetBSD: Makefile,v 1.1.1.7 1997/10/30 05:29:27 mrg Exp $

PROG=	ipmon
MAN=	ipmon.8
CPPFLAGS+=-DLOGFAC=LOG_LOCAL0 -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
