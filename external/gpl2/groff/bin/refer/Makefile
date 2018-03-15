# $NetBSD: Makefile,v 1.2.14.1 2018/03/15 09:11:55 pgoyette Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/refer/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

GROFFYSRCS=	label.y
DIST_SUBDIR=	src/preproc/refer
NOLIBDRIVER=

.include <bsd.init.mk>

PROG_CXX=	refer
SRCS=		label.cpp command.cpp ref.cpp refer.cpp token.cpp
CFLAGS+=	-I${DIST_DIR}
DPADD+=		${LIBBIB}
LDADD+=		${LIBBIB}
CLEANFILES+=	${MAN}

.include <bsd.prog.mk>
