# $NetBSD: Makefile,v 1.5 2006/10/09 08:32:53 mrg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/devices/grohtml/Makefile,v 1.5 2003/05/01 13:22:18 ru Exp $

DIST_SUBDIR=	src/devices/grohtml

PROG=		post-grohtml
MAN=		grohtml.1
SRCS=		post-html.cpp html-table.cpp html-text.cpp output.cpp
DPADD+=		${LIBDRIVER} ${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBDRIVER} ${LIBGROFF} -lm -lsupc++
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
