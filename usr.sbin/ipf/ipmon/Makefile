#	$NetBSD: Makefile,v 1.13 1997/10/25 06:58:11 lukem Exp $

PROG=	ipmon
MAN=	ipmon.8
CPPFLAGS+=-DLOGFAC=LOG_LOCAL0 -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
