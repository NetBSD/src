# $FreeBSD: src/gnu/usr.bin/groff/src/devices/grodvi/Makefile,v 1.4 2003/05/01 13:22:18 ru Exp $

DIST_SUBDIR=	src/devices/grodvi

PROG_CXX=	grodvi
SRCS=		dvi.cpp
DPADD+=		${LIBDRIVER} ${LIBGROFF} ${LIBM}
LDADD+=		${LIBDRIVER} ${LIBGROFF} -lm
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
