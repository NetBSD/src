# $NetBSD: Makefile,v 1.7 2003/10/20 00:54:37 lukem Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/eqn/Makefile,v 1.4 2003/05/01 13:22:19 ru Exp $

DIST_SUBDIR=	src/preproc/eqn

PROG_CXX=	eqn
SRCS=		main.cpp lex.cpp \
		box.cpp limit.cpp list.cpp over.cpp text.cpp \
		script.cpp mark.cpp other.cpp delim.cpp sqrt.cpp pile.cpp special.cpp
GROFFYSRCS=	eqn.y
CFLAGS+=	-I${DIST_DIR} -I.
DPADD+=		${LIBGROFF}
LDADD+=		${LIBGROFF}
SCRIPTS=	neqn
MAN=		eqn.1 neqn.1
CLEANFILES=	${SCRIPTS} ${MAN}

.include <bsd.prog.mk>
