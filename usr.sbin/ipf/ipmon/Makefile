#	$NetBSD: Makefile,v 1.8 1997/05/07 13:20:43 mrg Exp $

PROG=	ipmon
MAN=	ipmon.8
CFLAGS+=-DLOGFAC=LOG_LOCAL0 -I${.CURDIR}/../ipf

.PATH.c: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
