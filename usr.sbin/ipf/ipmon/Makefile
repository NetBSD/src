#	$NetBSD: Makefile,v 1.4 1997/03/29 20:07:26 thorpej Exp $

PROG=	ipmon
MAN=	ipmon.8
CFLAGS+=-DLOGFAC=LOG_LOCAL0 -I${.CURDIR}/../../../sbin/ipf

.PATH: .${CURDIR}/../../../sbin/ipf
.include <bsd.prog.mk>
