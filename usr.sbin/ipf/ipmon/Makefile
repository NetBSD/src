#	$NetBSD: Makefile,v 1.5 1997/04/13 14:51:52 mrg Exp $

PROG=	ipmon
MAN=	ipmon.8
CFLAGS+=-DLOGFAC=LOG_LOCAL0 -I${.CURDIR}/../ipf

.PATH: .${CURDIR}/../ipf
.include <bsd.prog.mk>
