#	$NetBSD: Makefile,v 1.7 1997/05/06 20:46:09 gwr Exp $

PROG=	ipmon
MAN=	ipmon.8
CFLAGS+=-DLOGFAC=LOG_LOCAL0 -I${.CURDIR}/../ipf

.PATH.c: ${CURDIR}/../ipf
.include <bsd.prog.mk>
