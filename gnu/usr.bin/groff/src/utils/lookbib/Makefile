# $NetBSD: Makefile,v 1.5.6.1 2012/10/30 18:58:32 yamt Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/utils/lookbib/Makefile,v 1.4 2003/05/01 13:22:21 ru Exp $

DIST_SUBDIR=	src/utils/lookbib

PROG_CXX=	lookbib
SRCS=		lookbib.cpp
DPADD+=		${LIBBIB}
LDADD+=		${LIBBIB}
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
