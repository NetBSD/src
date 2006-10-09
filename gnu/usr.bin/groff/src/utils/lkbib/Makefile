# $NetBSD: Makefile,v 1.4 2006/10/09 08:32:54 mrg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/utils/lkbib/Makefile,v 1.4 2003/05/01 13:22:21 ru Exp $

DIST_SUBDIR=	src/utils/lkbib

PROG=		lkbib
SRCS=		lkbib.cpp
DPADD+=		${LIBBIB} ${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBBIB} ${LIBGROFF} -lm -lsupc++
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
