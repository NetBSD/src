# $NetBSD: Makefile,v 1.3 2003/06/30 18:47:57 wiz Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/utils/tfmtodit/Makefile,v 1.4 2003/05/01 13:22:21 ru Exp $

DIST_SUBDIR=	src/utils/tfmtodit

PROG_CXX=	tfmtodit
SRCS=		tfmtodit.cpp
DPADD+=		${LIBGROFF} ${LIBM}
LDADD+=		${LIBGROFF} -lm
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
