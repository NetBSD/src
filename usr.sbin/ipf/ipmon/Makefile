#	$NetBSD: Makefile,v 1.1.1.5 1997/07/05 05:13:27 darrenr Exp $

PROG=	ipmon
MAN=	ipmon.8
CFLAGS+=-DLOGFAC=LOG_LOCAL0 -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
