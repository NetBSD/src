# $NetBSD: Makefile,v 1.1 2016/01/13 19:01:56 christos Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/eqn/Makefile,v 1.4 2003/05/01 13:22:19 ru Exp $

DIST_SUBDIR=	src/preproc/eqn
GROFFYSRCS=	eqn.y

.include <bsd.init.mk>

PROG_CXX=	eqn
SRCS=		main.cpp lex.cpp eqn.cpp \
		box.cpp limit.cpp list.cpp over.cpp text.cpp \
		script.cpp mark.cpp other.cpp delim.cpp sqrt.cpp pile.cpp special.cpp
CPPFLAGS+=	-I${DIST_DIR} -I.
SCRIPTS=	neqn
MAN=		eqn.1 neqn.1
CLEANFILES=	${SCRIPTS} ${MAN}

.include <bsd.prog.mk>
