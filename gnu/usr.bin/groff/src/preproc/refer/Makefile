# $NetBSD: Makefile,v 1.10 2010/12/16 22:54:21 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/refer/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/preproc/refer

PROG=		refer
SRCS=		command.cpp ref.cpp refer.cpp token.cpp
GROFFYSRCS=	label.y
CFLAGS+=	-I${DIST_DIR}
DPADD+=		${LIBBIB}
LDADD+=		${LIBBIB}
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
