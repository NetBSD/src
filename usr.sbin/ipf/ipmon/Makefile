#	$NetBSD: Makefile,v 1.12.2.1 1997/10/30 07:16:50 mrg Exp $

PROG=	ipmon
MAN=	ipmon.8
CPPFLAGS+=-DLOGFAC=LOG_LOCAL0 -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
