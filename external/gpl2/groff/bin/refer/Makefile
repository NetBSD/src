# $NetBSD: Makefile,v 1.1 2016/01/13 19:01:57 christos Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/refer/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

GROFFYSRCS=	label.y
DIST_SUBDIR=	src/preproc/refer

.include <bsd.init.mk>

PROG_CXX=	refer
SRCS=		label.cpp command.cpp ref.cpp refer.cpp token.cpp
CFLAGS+=	-I${DIST_DIR}
DPADD+=		${LIBBIB}
LDADD+=		${LIBBIB}
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
