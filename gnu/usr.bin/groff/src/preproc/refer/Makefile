# $NetBSD: Makefile,v 1.7 2003/08/01 10:36:43 lukem Exp $

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

label.cpp: label.y

.include <bsd.prog.mk>
