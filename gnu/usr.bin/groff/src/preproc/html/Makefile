# $NetBSD: Makefile,v 1.3 2003/06/30 18:47:51 wiz Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/html/Makefile,v 1.5 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/preproc/html

PROG_CXX=	pre-grohtml
SRCS=		pre-html.cpp pushback.cpp
DPADD+=		${LIBGROFF}
LDADD+=		${LIBGROFF}
NOMAN=		YES

.include <bsd.prog.mk>
