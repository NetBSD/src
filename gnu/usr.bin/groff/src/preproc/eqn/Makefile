# $NetBSD: Makefile,v 1.10 2012/08/10 12:12:19 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/eqn/Makefile,v 1.4 2003/05/01 13:22:19 ru Exp $

DIST_SUBDIR=	src/preproc/eqn

PROG_CXX=	eqn
SRCS=		main.cpp lex.cpp \
		box.cpp limit.cpp list.cpp over.cpp text.cpp \
		script.cpp mark.cpp other.cpp delim.cpp sqrt.cpp pile.cpp special.cpp
GROFFYSRCS=	eqn.y
CFLAGS+=	-I${DIST_DIR} -I.
SCRIPTS=	neqn
MAN=		eqn.1 neqn.1
CLEANFILES=	${SCRIPTS} ${MAN}

.include <bsd.prog.mk>
