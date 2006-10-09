# $NetBSD: Makefile,v 1.4 2006/10/09 08:32:53 mrg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/devices/grodvi/Makefile,v 1.4 2003/05/01 13:22:18 ru Exp $

DIST_SUBDIR=	src/devices/grodvi

PROG=		grodvi
SRCS=		dvi.cpp
DPADD+=		${LIBDRIVER} ${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBDRIVER} ${LIBGROFF} -lm -lsupc++
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
