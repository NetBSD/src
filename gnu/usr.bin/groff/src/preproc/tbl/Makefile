# $NetBSD: Makefile,v 1.3 2003/06/30 18:47:53 wiz Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/tbl/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/preproc/tbl

PROG_CXX=	tbl
SRCS=		main.cpp table.cpp
DPADD+=		${LIBGROFF} ${LIBM}
LDADD+=		${LIBGROFF} -lm
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
