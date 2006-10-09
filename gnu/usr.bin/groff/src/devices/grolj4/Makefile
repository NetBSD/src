# $NetBSD: Makefile,v 1.5 2006/10/09 08:32:54 mrg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/devices/grolj4/Makefile,v 1.4 2003/05/01 13:22:18 ru Exp $

DIST_SUBDIR=	src/devices/grolj4

PROG=		grolj4
SRCS=		lj4.cpp
MAN=		grolj4.1 lj4_font.5
DPADD+=		${LIBDRIVER} ${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBDRIVER} ${LIBGROFF} -lm -lsupc++
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
