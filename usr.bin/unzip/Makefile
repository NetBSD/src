# $NetBSD: Makefile,v 1.1 2009/06/25 20:27:05 joerg Exp $

PROG=	unzip

DPADD+=		${LIBARCHIVE} ${LIBZ} ${LIBBZ2}
LDADD+=		-larchive -lz -lbz2

.include <bsd.prog.mk>
