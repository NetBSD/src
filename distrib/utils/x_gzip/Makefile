#	$NetBSD: Makefile,v 1.3 2000/05/18 01:10:28 matt Exp $

PROG=	gzip
LDADD=	-lz
DPADD=	${LIBZ}
COPTS+= -Os
LDSTATIC?=-static

.include <bsd.prog.mk>
