# $NetBSD: Makefile,v 1.1 2016/01/13 19:01:56 christos Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/utils/lkbib/Makefile,v 1.4 2003/05/01 13:22:21 ru Exp $

DIST_SUBDIR=	src/utils/lkbib

.include <bsd.init.mk>

PROG_CXX=	lkbib
SRCS=		lkbib.cpp
DPADD+=		${LIBBIB}
LDADD+=		${LIBBIB}
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
