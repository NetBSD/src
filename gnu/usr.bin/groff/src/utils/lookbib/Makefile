# $NetBSD: Makefile,v 1.4 2006/10/09 08:32:55 mrg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/utils/lookbib/Makefile,v 1.4 2003/05/01 13:22:21 ru Exp $

DIST_SUBDIR=	src/utils/lookbib

PROG=		lookbib
SRCS=		lookbib.cpp
DPADD+=		${LIBBIB} ${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBBIB} ${LIBGROFF} -lm -lsupc++
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
