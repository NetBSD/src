#	$NetBSD: Makefile,v 1.3 1997/03/29 19:49:05 thorpej Exp $

PROG=	ipmon
MAN=	ipmon.8
CFLAGS+=-DLOGFAC=LOG_LOCAL0 -I${.CURDIR}/../../../sbin/ipf -I${.CURDIR}/../../../sys

.PATH: .${CURDIR}/../../../sbin/ipf
.include <bsd.prog.mk>
