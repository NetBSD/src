# $NetBSD: Makefile,v 1.10.6.1 2012/10/30 18:58:31 yamt Exp $

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
