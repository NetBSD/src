# $NetBSD: Makefile,v 1.3 2003/06/30 18:47:48 wiz Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/devices/grolbp/Makefile,v 1.4 2003/05/01 13:22:18 ru Exp $

DIST_SUBDIR=	src/devices/grolbp

PROG_CXX=	grolbp
SRCS=		lbp.cpp
DPADD+=		${LIBDRIVER} ${LIBGROFF} ${LIBM}
LDADD+=		${LIBDRIVER} ${LIBGROFF} -lm
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
