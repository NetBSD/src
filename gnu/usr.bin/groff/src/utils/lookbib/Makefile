# $NetBSD: Makefile,v 1.6 2012/08/10 12:12:22 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/utils/lookbib/Makefile,v 1.4 2003/05/01 13:22:21 ru Exp $

DIST_SUBDIR=	src/utils/lookbib

PROG_CXX=	lookbib
SRCS=		lookbib.cpp
DPADD+=		${LIBBIB}
LDADD+=		${LIBBIB}
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
