# $NetBSD: Makefile,v 1.4 2006/10/09 08:32:54 mrg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/roff/groff/Makefile,v 1.7 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/roff/groff

PROG=		groff
SRCS=		groff.cpp pipeline.c
DPADD+=		${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBGROFF} -lm -lsupc++
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
