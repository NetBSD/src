#	$NetBSD: Makefile,v 1.12 1997/09/21 18:01:47 veego Exp $

PROG=	ipmon
MAN=	ipmon.8
CFLAGS+=-DLOGFAC=LOG_LOCAL0 -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
