# $NetBSD: Makefile,v 1.3 2003/06/30 18:47:51 wiz Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/grn/Makefile,v 1.4 2003/05/01 13:22:19 ru Exp $

DIST_SUBDIR=	src/preproc/grn

PROG_CXX=	grn
SRCS=		hdb.cpp hpoint.cpp hgraph.cpp main.cpp
DPADD+=	 	${LIBGROFF} ${LIBM}
LDADD+=	 	${LIBGROFF} -lm
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
