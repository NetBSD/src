#	$NetBSD: Makefile,v 1.9 2007/05/28 12:06:31 tls Exp $

USE_FORT?= yes	# network client

PROG = rwall
WARNS=3

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

.include <bsd.prog.mk>
