# $NetBSD: Makefile,v 1.6 2003/07/01 08:20:37 mrg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/refer/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/preproc/refer

PROG_CXX=	refer
SRCS=		command.cpp label.cpp ref.cpp refer.cpp token.cpp
CFLAGS+=	-I${DIST_DIR}
DPADD+=		${LIBBIB} ${LIBGROFF} ${LIBM}
LDADD+=		${LIBBIB} ${LIBGROFF} -lm
# YHEADER is useless here; label.y -> label_tab.h and label.cpp.
YFLAGS=		-d
CLEANFILES=	label.cpp label_tab.h ${MAN} y.tab.c y.tab.h

.include <bsd.prog.mk>
