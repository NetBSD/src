# $NetBSD: Makefile,v 1.3 2003/06/30 18:47:55 wiz Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/utils/addftinfo/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/utils/addftinfo

PROG_CXX=	addftinfo
SRCS=		addftinfo.cpp guess.cpp
DPADD+=		${LIBGROFF}
LDADD+=		${LIBGROFF}
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
