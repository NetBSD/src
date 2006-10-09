# $NetBSD: Makefile,v 1.4 2006/10/09 08:32:54 mrg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/devices/grops/Makefile,v 1.4 2003/05/01 13:22:19 ru Exp $

DIST_SUBDIR=	src/devices/grops

PROG=		grops
SRCS=		ps.cpp psrm.cpp
DPADD+=		${LIBDRIVER} ${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBDRIVER} ${LIBGROFF} -lm -lsupc++
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
