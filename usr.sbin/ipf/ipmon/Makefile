#	$NetBSD: Makefile,v 1.6 1997/05/04 09:52:49 veego Exp $

PROG=	ipmon
MAN=	ipmon.8
CFLAGS+=-DLOGFAC=LOG_LOCAL0 -I${.CURDIR}/../ipf

.PATH: ${CURDIR}/../ipf
.include <bsd.prog.mk>
