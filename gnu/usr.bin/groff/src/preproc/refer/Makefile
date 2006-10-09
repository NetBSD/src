# $NetBSD: Makefile,v 1.9 2006/10/09 08:32:54 mrg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/refer/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/preproc/refer

PROG=		refer
SRCS=		command.cpp ref.cpp refer.cpp token.cpp
GROFFYSRCS=	label.y
CFLAGS+=	-I${DIST_DIR}
DPADD+=		${LIBBIB} ${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBBIB} ${LIBGROFF} -lm -lsupc++
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
