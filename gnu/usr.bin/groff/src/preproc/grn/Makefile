# $NetBSD: Makefile,v 1.4 2006/10/09 08:32:54 mrg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/grn/Makefile,v 1.4 2003/05/01 13:22:19 ru Exp $

DIST_SUBDIR=	src/preproc/grn

PROG=		grn
SRCS=		hdb.cpp hpoint.cpp hgraph.cpp main.cpp
DPADD+=	 	${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=	 	${LIBGROFF} -lm -lsupc++
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
