#	$NetBSD: Makefile,v 1.1.1.11 1998/11/22 14:21:54 mrg Exp $

PROG=	ipmon
MAN=	ipmon.8
CPPFLAGS+=-DLOGFAC=LOG_LOCAL0 -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
