# $NetBSD: Makefile,v 1.5 2008/02/26 03:20:07 lukem Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/devices/grolbp/Makefile,v 1.4 2003/05/01 13:22:18 ru Exp $

DIST_SUBDIR=	src/devices/grolbp

PROG=		grolbp
SRCS=		lbp.cpp
DPADD+=		${LIBDRIVER} ${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBDRIVER} ${LIBGROFF} -lm -lsupc++
CLEANFILES=	${MAN}
USETBL=

.include <bsd.prog.mk>
