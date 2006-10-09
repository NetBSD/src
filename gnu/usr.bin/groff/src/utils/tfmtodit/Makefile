# $NetBSD: Makefile,v 1.4 2006/10/09 08:32:55 mrg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/utils/tfmtodit/Makefile,v 1.4 2003/05/01 13:22:21 ru Exp $

DIST_SUBDIR=	src/utils/tfmtodit

PROG=		tfmtodit
SRCS=		tfmtodit.cpp
DPADD+=		${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBGROFF} -lm -lsupc++
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
