# $NetBSD: Makefile,v 1.11 2012/08/10 12:12:20 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/refer/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/preproc/refer

PROG_CXX=	refer
SRCS=		command.cpp ref.cpp refer.cpp token.cpp
GROFFYSRCS=	label.y
CFLAGS+=	-I${DIST_DIR}
DPADD+=		${LIBBIB}
LDADD+=		${LIBBIB}
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
