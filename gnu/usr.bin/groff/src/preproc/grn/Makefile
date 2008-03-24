# $NetBSD: Makefile,v 1.4.14.1 2008/03/24 07:14:43 keiichi Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/grn/Makefile,v 1.4 2003/05/01 13:22:19 ru Exp $

DIST_SUBDIR=	src/preproc/grn

PROG=		grn
SRCS=		hdb.cpp hpoint.cpp hgraph.cpp main.cpp
DPADD+=	 	${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=	 	${LIBGROFF} -lm -lsupc++
CLEANFILES=	${MAN}
USETBL=

.include <bsd.prog.mk>
