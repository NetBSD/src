# $NetBSD: Makefile,v 1.4 2006/10/09 08:32:54 mrg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/html/Makefile,v 1.5 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/preproc/html

PROG=		pre-grohtml
SRCS=		pre-html.cpp pushback.cpp
DPADD+=		${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBGROFF} -lm -lsupc++
NOMAN=		YES

.include <bsd.prog.mk>
