#	$NetBSD: Makefile,v 1.1.1.9 1998/05/17 16:29:51 veego Exp $

PROG=	ipmon
MAN=	ipmon.8
CPPFLAGS+=-DLOGFAC=LOG_LOCAL0 -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
